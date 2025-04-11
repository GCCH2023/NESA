#include "stdafx.h"
#include "ReachingDefinition.h"
using namespace std;




void DumpBasicBlockDefinitions(TACBasicBlock* block, TacAxyDefinition& axyDefs, TACFunction* tacSub)
{
	vector<TAC*> codes(32);
	auto blockSet = (BasicBlockReachingDefinitionSet*)block->tag;
	Sprintf<> s;
	COUT << s.Format(_T("block %04X:\nIN:\n"), block->GetStartAddress());
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

ReachingDefinition::ReachingDefinition(NesDataBase& db, Allocator& allocator):
TACFunctionAnalyzer(db, allocator)
{

}

ReachingDefinition::~ReachingDefinition()
{
}

bool ReachingDefinition::CanReach(const Set& set, int var)
{
	assert(var >= TAC_REG_A && var <= TAC_REG_C);
	return (set & masks[var]).any();
}


// 输出AXY的所有定值三地址码
void DumpAXYDefinitions(TacAxyDefinition& axyDefs, TACFunction* tacSub)
{
	auto codes = tacSub->GetCodes();
	COUT << _T("获取AXYNVZC的所有定值点：\n");
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		COUT << ToString((TACRegister)i) << _T(":\n");
		for (auto d : axyDefs.defs[i])
		{
			DumpAddressTAC(COUT, codes[d]) << endl;
		}
	}
}

// 获取三地址码中的所有对A, X, Y, NVZC 寄存器的定值点
void ReachingDefinition::GetDefinitions(TacAxyDefinition& axyDefs)
{
	// 不用记录所有的定值点，每个基本块，只需要记录变量最后的定值点
	int i = 0;
	for (auto block : GetFunction()->GetBasicBlocks())
	{
		std::vector<int> defsMap(TAC_ANALIZE_REG_COUNT, -1);  // 当前基本块，每个变量的定值点，-1表示未定值
		for (auto code : block->GetCodes())
		{
			// 函数调用也可能给AXY定值
			if (code->op == TACOperator::CALL)
			{
				auto sub = db.FindSubroutine(code->z.GetValue());
				if (!sub)
					continue;
				if (sub->flag & SUBF_RETURN_A)
					defsMap[TAC_REG_A] = i;
				if (sub->flag & SUBF_RETURN_X)
					defsMap[TAC_REG_X] = i;
				if (sub->flag & SUBF_RETURN_Y)
					defsMap[TAC_REG_Y] = i;
			}
			else if (code->op == TACOperator::ARRAY_SET)
			{
				// 对数组元素赋值不要作为z的定值
			}
			else
			{
				if (IsAxyNvzc(code->z))
					defsMap[code->z.GetValue()] = i;
			}
			++i;
		}
		for (int j = 0; j < TAC_ANALIZE_REG_COUNT; ++j)
		{
			if (defsMap[j] != -1)
			{
				axyDefs.defs[j].push_back(defsMap[j]);
			}
		}
	}
	DumpAXYDefinitions(axyDefs, GetFunction());
}

void ReachingDefinition::GenerateMask()
{
	// 先计算定值点总数
	count = 0;
	for (auto& defs : axyDefs.defs)
	{
		count += defs.size();
	}

	int start = 0, end = 0;
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		end = start + (int)axyDefs.defs[i].size();
		masks[i].resize(count);
		if (start == end)
			continue;
		masks[i].set(start, end - start, true); // 构造目标区间的掩码
		start = end;
	}
}

void ReachingDefinition::Uninitialize()
{
	DumpAllBasicBlockDefinitions(axyDefs, GetFunction());
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
void ReachingDefinition::GetBasickBlockGenKillMap()
{
	auto codes = GetFunction()->GetCodes();
	// 依次遍历 AXYNVZC 寄存器的定值点，获取对应的基本块来设置它的生成杀死集
	// 每个变量只有最后一次定值可以导出基本块的出口，所以要清除其他定值点
	size_t index = 0;
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		for (auto d : axyDefs.defs[i])
		{
			auto code = codes[d];
			TACBasicBlock* block = GetBasickBlockByAddress(GetFunction()->GetBasicBlocks(), code->address);
			auto tag = (BasicBlockReachingDefinitionSet*)block->tag;
			tag->gen[index] = true;  // 生成这个定值点
			tag->kill |= masks[i];  // 杀死这个变量除此之外的定值点
			tag->kill[index] = false;
			++index;
		}
	}
}

void ReachingDefinition::Initialize()
{
	// 获取变量的定值点
	GetDefinitions(axyDefs);
	GenerateMask();

	// 创建分析对象
	auto& blocks = GetFunction()->GetBasicBlocks();
	for (auto block : blocks)
	{
		auto tag = allocator.New<BasicBlockReachingDefinitionSet>();
		tag->gen.resize(count);
		tag->kill.resize(count);
		tag->in.resize(count);
		tag->out.resize(count);
		block->tag = tag;
	}

	// 接下来计算各个基本块的生成集和杀死集
	GetBasickBlockGenKillMap();
}

bool ReachingDefinition::AnalyzeNode(TACBasicBlock* block)
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

// 有一个集合变化，就认为变化
bool BasicBlockReachingDefinitionSet::EvalOut()
{
	auto old = out;
	out = gen | (in - kill);
	return out != old;
}

void TacAxyDefinition::CheckDefinitionLimit(TACFunction* tacSub)
{
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		if (defs[i].size() > MAX_NODE)
		{
			Sprintf<> s;
			s.Format(_T("对子程序 %04X 进行到达定值分析时，变量的定值点超过容量 %d"),
				tacSub->GetStartAddress(), MAX_NODE);
			throw Exception(s.ToString());
		}
	}
}

void TacAxyDefinition::GetDefinitionTACList(std::vector<TAC*>& result, Set& set, TACFunction* tacSub)
{
	result.clear();
	auto codes = tacSub->GetCodes();
	size_t index = 0;
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		for (auto def : defs[i])
		{
			if (set[index])
				result.push_back(codes[def]);
			++index;
		}
	}
}
