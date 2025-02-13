#include "stdafx.h"
#include "ReachingDefinition.h"
#include "TAC.h"
#include "NesDataBase.h"
using namespace std;
#include "TACTranslater.h"




void DumpBasicBlockDefinitions(TACBasicBlock* block, TacAxyDefinition& axyDefs, TACSubroutine* tacSub)
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


void DumpAllBasicBlockDefinitions(TacAxyDefinition& axyDefs, TACSubroutine* tacSub)
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
void DumpAXYDefinitions(TacAxyDefinition& axyDefs, TACSubroutine* tacSub)
{
	auto& codes = tacSub->GetCodes();
	COUT << "获取AXY的所有定值点：\n";
	COUT << "A:\n";
	for (auto i : axyDefs.adefs)
	{
		DumpAddressTAC(COUT, codes[i]) << endl;
	}
	COUT << "X:\n";
	for (auto i : axyDefs.xdefs)
	{
		DumpAddressTAC(COUT, codes[i]) << endl;
	}
	COUT << "Y:\n";
	for (auto i : axyDefs.ydefs)
	{
		DumpAddressTAC(COUT, codes[i]) << endl;
	}
}

// 获取三地址码中的所有对A, X, Y 寄存器的定值点
void ReachingDefinition::GetAXYDefinitions(TacAxyDefinition& axyDefs, TACSubroutine* tacSub)
{
	int i = 0;
	for (auto code : tacSub->GetCodes())
	{
		// 函数调用也可能给AXY定值
		if (code->op == TACOperator::CALL)
		{
			auto sub = db.FindSubroutine(code->z.GetValue());
			if (sub->flag & SUBF_RTURN_A)
				axyDefs.adefs.push_back(i);
			if (sub->flag & SUBF_RTURN_X)
				axyDefs.xdefs.push_back(i);
			if (sub->flag & SUBF_RTURN_Y)
				axyDefs.ydefs.push_back(i);
			continue;
		}
		if (code->z.IsRegister())
		{
			switch (code->z.GetValue())
			{
			case  Nes::NesRegisters::A:
				axyDefs.adefs.push_back(i);
				break;
			case  Nes::NesRegisters::X:
				axyDefs.xdefs.push_back(i);
				break;
			case  Nes::NesRegisters::Y:
				axyDefs.ydefs.push_back(i);
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
void GetBasickBlockGenKillMap(TacAxyDefinition& axyDefs, TACSubroutine* tacSub,
	const TACBasicBlockList& blocks)
{
	auto codes = tacSub->GetCodes();
	// 依次遍历 AXY 寄存器的定值点，获取对应的基本块来设置它的生成杀死集
	size_t index = 0;
	for (auto d : axyDefs.adefs)
	{
		auto code = codes[d];
		TACBasicBlock* block = GetBasickBlockByAddress(blocks, code->address);
		((BasicBlockReachingDefinitionSet*)block->tag)->genKill.a = 1 << index;  // 清空
		++index;
	}
	index = 0;
	for (auto d : axyDefs.xdefs)
	{
		auto code = codes[d];
		TACBasicBlock* block = GetBasickBlockByAddress(blocks, code->address);
		((BasicBlockReachingDefinitionSet*)block->tag)->genKill.x = 1 << index;  // 清空
		++index;
	}
	index = 0;
	for (auto d : axyDefs.ydefs)
	{
		auto code = codes[d];
		TACBasicBlock* block = GetBasickBlockByAddress(blocks, code->address);
		((BasicBlockReachingDefinitionSet*)block->tag)->genKill.y = 1 << index;  // 清空
		++index;
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
	a |= other.a;
	x |= other.x;
	y |= other.y;
	return *this;
}

bool BasicBlockReachingDefinitionSet::EvalOut()
{
	auto value = out.a;
	bool ret = false;
	out.a = genKill.a.Any() ? genKill.a : in.a;
	if (out.a != value)
		ret = true;
	value = out.x;
	out.x = genKill.x.Any() ? genKill.x : in.x;
	if (out.x != value)
		ret = true;
	value = out.y;
	out.y = genKill.y.Any() ? genKill.y : in.y;
	if (out.y != value)
		ret = true;
	return ret;
}

void TacAxyDefinition::CheckDefinitionLimit()
{
	if (adefs.size() > 32 || xdefs.size() > 32 || ydefs.size() > 32)
		throw Exception(_T("对子程序进行到达定值分析时，AXY中的一个定值点数量超过32\n"));
}

void TacAxyDefinition::GetDefinitionTACList(std::vector<TAC*>& result, AXYSet& set, TACSubroutine* tacSub)
{
	result.clear();
	auto& codes = tacSub->GetCodes();
	auto list = set.a.ToVector();
	for (auto i : list)
		result.push_back(codes[adefs[i]]);
	list = set.x.ToVector();
	for (auto i : list)
		result.push_back(codes[xdefs[i]]);
	list = set.y.ToVector();
	for (auto i : list)
		result.push_back(codes[ydefs[i]]);
}
