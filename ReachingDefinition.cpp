#include "stdafx.h"
#include "ReachingDefinition.h"
#include "TACFunction.h"
#include "NesDataBase.h"
using namespace std;




void DumpBasicBlockDefinitions(TACBasicBlock* block, TacAxyDefinition& axyDefs, TACFunction* tacSub)
{
	vector<TAC*> codes(32);
	TCHAR buffer[64];
	auto blockSet = (BasicBlockReachingDefinitionSet*)block->tag;
	_stprintf_s(buffer, 64, _T("block %04X:\nIN:\n"), block->GetStartAddress());
	COUT << buffer;
	axyDefs.GetDefinitionTACList(codes, blockSet->in, tacSub);
	for (auto tac : codes)
		DumpAddressTAC(COUT, tac) << endl;
	COUT << "OUT:\n";
	axyDefs.GetDefinitionTACList(codes, blockSet->out, tacSub);
	for (auto tac : codes)
		DumpAddressTAC(COUT, tac) << endl;
}


void DumpAllBasicBlockDefinitions(TacAxyDefinition& axyDefs, TACFunction* tacSub)
{
	for (auto block : tacSub->GetBasicBlocks())
	{
		DumpBasicBlockDefinitions(block, axyDefs, tacSub);
	}
}

ReachingDefinition::ReachingDefinition(NesDataBase& db_, Allocator& allocator_):
DataFlowAnalyzer(db_),
allocator(allocator_)
{

}

ReachingDefinition::~ReachingDefinition()
{
}


// 输出AXY的所有定值三地址码
void DumpAXYDefinitions(TacAxyDefinition& axyDefs, TACFunction* tacSub)
{
	auto& codes = tacSub->GetCodes();
	COUT << _T("获取AXY的所有定值点：\n");
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		COUT << ToString((TACRegister)i) << _T(":\n");
		DumpAddressTAC(COUT, codes[i]) << endl;
	}
}

// 获取三地址码中的所有对A, X, Y 寄存器的定值点
void ReachingDefinition::GetAXYDefinitions(TacAxyDefinition& axyDefs, TACFunction* tacSub)
{
	int i = 0;
	for (auto code : tacSub->GetCodes())
	{
		// 函数调用也可能给AXY定值
		if (code->op == TACOperator::CALL)
		{
			auto sub = db.FindSubroutine(code->z.GetValue());
			if (!sub)
				continue;
			if (sub->flag & SUBF_RETURN_A)
				axyDefs.defs[TAC_REG_A].push_back(i);
			if (sub->flag & SUBF_RETURN_X)
				axyDefs.defs[TAC_REG_X].push_back(i);
			if (sub->flag & SUBF_RETURN_Y)
				axyDefs.defs[TAC_REG_Y].push_back(i);
			continue;
		}
		if (code->z.IsRegister())
		{
			switch (code->z.GetValue())
			{
			case  TAC_REG_A:
				axyDefs.defs[TAC_REG_A].push_back(i);
				break;
			case  TAC_REG_X:
				axyDefs.defs[TAC_REG_X].push_back(i);
				break;
			case  TAC_REG_Y:
				axyDefs.defs[TAC_REG_Y].push_back(i);
				break;
			}
		}
		++i;
	}
	//DumpAXYDefinitions(axyDefs, tacSub);
}

void ReachingDefinition::Uninitialize()
{
	// DumpAllBasicBlockDefinitions(axyDefs, this->subroutine);
}

TACBasicBlock* GetBasickBlockByAddress(const TACBasicBlockList& blocks, Nes::Address address)
{
	for (auto block : blocks)
	{
		if (address < block->GetEndAddress())
			return block;
	}
	throw Exception(_T("找不到指定地址的基本块"));
	// 查找第一个结束地址大于指定地址的基本块
	// !! 不知道下面的代码有什么问题，结果就是不对
	/*auto it = std::upper_bound(blocks.begin(), blocks.end(), address,
		[](Nes::Address a, BasicBlock* b) {
		std::COUT << a << " < " << b->GetEndAddress() << " : " << (a < b->GetEndAddress()) << std::endl;
		return a < b->GetEndAddress();
		});
		if (it != blocks.end())
		*it;
		return nullptr;*/
}

// 计算各个基本块的生成杀死集
// 每个变量的定值点，杀死这个变量的其他定值点
// 对于一个基本块，每个变量最多只有一个定值点可以到达出口
void GetBasickBlockGenKillMap(TacAxyDefinition& axyDefs, TACFunction* tacSub,
	const TACBasicBlockList& blocks)
{
	auto codes = tacSub->GetCodes();
	// 依次遍历 AXY 寄存器的定值点，获取对应的基本块来设置它的生成杀死集
	// 每个变量只有最后一次定值可以导出基本块的出口，所以要清除其他定值点
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		size_t index = 0;
		for (auto d : axyDefs.defs[i])
		{
			auto code = codes[d];
			TACBasicBlock* block = GetBasickBlockByAddress(blocks, code->address);
			((BasicBlockReachingDefinitionSet*)block->tag)->genKill.set[i] = 1 << index;  // 清空
			++index;
		}
	}
}

void ReachingDefinition::Initialize()
{
	auto& blocks = this->subroutine->GetBasicBlocks();
	for (auto block : blocks)
	{
		block->tag = allocator.New<BasicBlockReachingDefinitionSet>();
	}

	// 只分析寄存器 A, X, Y，其他寄存器和临时变量或者全局变量忽略掉
	GetAXYDefinitions(axyDefs, this->subroutine);
	axyDefs.CheckDefinitionLimit();

	// 接下来计算各个基本块的生成集和杀死集
	GetBasickBlockGenKillMap(axyDefs, this->subroutine, blocks);
}

bool ReachingDefinition::IteraterBasicBlock(TACBasicBlock* block)
{
	auto blockSet = (BasicBlockReachingDefinitionSet*)block->tag;
	// IN[B] = 所有前驱的定值点的并集
	// OUT[B] = IN[B] 并 gen_kill(B)
	for (auto prev : block->prevs)
	{
		auto prevSet = (BasicBlockReachingDefinitionSet*)prev->tag;
		blockSet->in |= prevSet->out;
	}
	return !blockSet->EvalOut();
}


AXYSet& AXYSet::operator|=(const AXYSet& other)
{
	for (int i = 0; i < TAC_ANALIZE_REG_COUNT; ++i)
	{
		set[i] |= other.set[i];
	}
	return *this;
}

// 有一个集合变化，就认为变化
bool BasicBlockReachingDefinitionSet::EvalOut()
{
	bool ret = false;
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		auto value = out.set[i];
		// 对于每个变量，如果这个基本块有它的定值，则出口集就是这个定值点
		// 否则，出口集是它的入口集
		out.set[i] = genKill.set[i].Any() ? genKill.set[i] : in.set[i];
		if (out.set[i] != value)
			ret = true;
	}
	return ret;
}

void TacAxyDefinition::CheckDefinitionLimit()
{
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		if (defs[i].size() > MAX_NODE)
		{
			Sprintf<> s;
			s.Format(_T("对子程序进行到达定值分析时，变量的定值点超过容量 %d"), MAX_NODE);
			throw Exception(s.ToString());
		}
	}
}

void TacAxyDefinition::GetDefinitionTACList(std::vector<TAC*>& result, AXYSet& set, TACFunction* tacSub)
{
	result.clear();
	auto& codes = tacSub->GetCodes();
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		auto list = set.set[i].ToVector();
		for (auto d : list)
		{
			auto& varDefs = defs[i];
			auto index = varDefs[d];
			result.push_back(codes[index]);
		}
	}
}
