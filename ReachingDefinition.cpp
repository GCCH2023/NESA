#include "stdafx.h"
#include "ReachingDefinition.h"
using namespace std;


void ReachingDefinitionResult::GetDefinitionTACList(std::vector<TAC*>& result, const Set& set) const
{
	result.clear();
	auto codes = function->GetCodes();
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

void ReachingDefinitionResult::GetBasicBlockDefinitionsIn(std::vector<TAC*>& result, size_t blockIndex) const
{
	return GetDefinitionTACList(result, data[blockIndex].in);
}


ReachingDefinitionResult::ReachingDefinitionResult(TACFunction* func) :
	function(func),
	count(0),
	data(func->GetBasicBlocks().size())
{
}

void ReachingDefinitionResult::DumpBasicBlockDefinitions(size_t index) const
{
	vector<TAC*> codes(32);
	auto block = function->GetBasicBlocks()[index];
	auto blockSet = (BasicBlockReachingDefinitionSet*)block->tag;
	Sprintf<> s;
	COUT << s.Format(_T("block %04X:\nIN:\n"), block->GetStartAddress());
	GetDefinitionTACList(codes, blockSet->in);
	for (auto tac : codes)
		DumpAddressTAC(COUT, tac) << endl;
	COUT << "OUT:\n";
	GetDefinitionTACList(codes, blockSet->out);
	for (auto tac : codes)
		DumpAddressTAC(COUT, tac) << endl;
}


void ReachingDefinitionResult::DumpAllBasicBlockDefinitions() const
{
	size_t index = 0;
	for (auto block : function->GetBasicBlocks())
	{
		DumpBasicBlockDefinitions(index++);
	}
}

void ReachingDefinitionResult::DumpRegisterDefinitions() const
{
	auto codes = function->GetCodes();
	COUT << _T("获取AXYNVZC的所有定值点：\n");
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		COUT << ToString((TACRegister)i) << _T(":\n");
		for (auto d : defs[i])
		{
			DumpAddressTAC(COUT, codes[d]) << endl;
		}
	}
}

bool ReachingDefinitionResult::CanReach(size_t blockIndex, bool isIn, int var)
{
	assert(var >= TAC_REG_A && var <= TAC_REG_C);
	if (isIn)
		return (data[blockIndex].in & masks[var]).any();
	else
		return (data[blockIndex].out & masks[var]).any();
}

void ReachingDefinitionResult::GenerateMask()
{
	// 先计算定值点总数
	count = 0;
	for (auto& defs : defs)
	{
		count += defs.size();
	}

	int start = 0, end = 0;
	for (int i = TAC_REG_A; i <= TAC_REG_C; ++i)
	{
		end = start + (int)defs[i].size();
		masks[i].resize(count);
		if (start == end)
			continue;
		masks[i].set(start, end - start, true); // 构造目标区间的掩码
		start = end;
	}

	for (auto& block : data)
	{
		block.gen.resize(count);
		block.kill.resize(count);
		block.in.resize(count);
		block.out.resize(count);
	}
}


ReachingDefinition::ReachingDefinition(NesDataBase& db, Allocator& allocator):
TACFunctionAnalyzer(db, allocator)
{

}

ReachingDefinition::~ReachingDefinition()
{
}

// 获取三地址码中的所有对A, X, Y, NVZC 寄存器的定值点
void ReachingDefinition::GetDefinitions()
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
				result->defs[j].push_back(defsMap[j]);
			}
		}
	}
	// DumpAXYDefinitions(axyDefs, GetFunction());
}


void ReachingDefinition::Uninitialize()
{
	// DumpAllBasicBlockDefinitions(axyDefs, GetFunction());
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
		for (auto d : result->defs[i])
		{
			auto code = codes[d];
			TACBasicBlock* block = GetBasickBlockByAddress(GetFunction()->GetBasicBlocks(), code->address);
			auto blockIndex = (size_t)block->tag;
			result->data[blockIndex].gen[index] = true;  // 生成这个定值点
			result->data[blockIndex].kill |= result->masks[i];  // 杀死这个变量除此之外的定值点
			result->data[blockIndex].kill[index] = false;
			++index;
		}
	}
}

void ReachingDefinition::Initialize()
{
	SetResult(std::make_shared<ReachingDefinitionResult>(GetFunction()));
	// 获取变量的定值点
	GetDefinitions();
	result->GenerateMask();

	// 创建分析对象
	auto& blocks = GetFunction()->GetBasicBlocks();
	size_t index = 0;
	for (auto block : blocks)
	{
		block->tag = (void*)index++;
	}

	// 接下来计算各个基本块的生成集和杀死集
	GetBasickBlockGenKillMap();
}

bool ReachingDefinition::AnalyzeNode(TACBasicBlock* block)
{
	auto blockIndex = (size_t)block->tag;
	// IN[B] = 所有前驱的定值点的并集
	// OUT[B] = IN[B] 并 gen_kill(B)
	for (auto prev : block->prevs)
	{
		auto prevIndex = (size_t)prev->tag;
		result->data[blockIndex].in |= result->data[prevIndex].out;
	}
	return result->data[blockIndex].EvalOut();
}

// 有一个集合变化，就认为变化
bool ReachingDefinitionResult::BasicBlockReachingDefinitionSet::EvalOut()
{
	auto old = out;
	out = gen | (in - kill);
	return out != old;
}