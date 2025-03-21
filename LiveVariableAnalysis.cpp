#include "stdafx.h"
#include "LiveVariableAnalysis.h"
#include "TACFunction.h"
#include "NesDataBase.h"
using namespace std;


LiveVariableAnalysis::LiveVariableAnalysis(NesDataBase& db, Allocator& allocator):
TACFunctionAnalyzer(db, allocator)
{

}

// 获取集合包含的变量的字符串
template<size_t N>
void FormatVariableSet(Sprintf<N>& s, const NodeSet& n)
{
	for (auto i : n.ToVector())
	{
		if (i < TAC_ANALIZE_REG_COUNT)
		{
			s.Append(ToString((TACRegister)i));
			s.Append(_T(", "));
		}
		else
		{
			s.Append(_T("t%d, "), i);
		}
	}
}

// 输出每个基本块的入口活跃变量集和出口活跃变量集
void DumpAllBasicBlockLiveVariables(TACBasicBlockList& blocks)
{
	Sprintf<512> s;
	s.Format(_T("基本块的活跃变量集：\n"));
	for (auto block : blocks)
	{
		s.Append(_T("基本块 %04X IN : "), block->GetStartAddress());
		auto blockSet = (BasicBlockLiveVariableSet*)block->tag;
		if (blockSet->in.None())
		{
			s.Append(_T("无 "));
		}
		else
		{
			FormatVariableSet(s, blockSet->in);
		}
		s.Append(_T(" OUT: "));
		if (blockSet->out.None())
		{
			s.Append(_T("无 "));
		}
		else
		{
			FormatVariableSet(s, blockSet->out);
		}
		s.Append(_T("\n"));
	}
	COUT << s.ToString();
}

// 分析寄存器 AXY 的引用（定义或使用）情况
void AnalyzeReference(TACOperand& operand, NodeSet& defs, NodeSet& uses, NodeSet& state)
{
	if (operand.IsRegister())
	{
		int index = operand.GetValue();
		if (index <= TAC_REG_C)
		{
			if (!defs.Contains(index))  // 使用前没有定值
			{
				state += index;
			}
			uses += index;  // 标记使用
		}
	}
	else if (operand.IsTemp())
	{
		int index = operand.GetValue() + TAC_ANALIZE_REG_COUNT;
		if (index >= MAX_NODE)
		{
			Sprintf<> s;
			s.Format(_T("对子程序进行活跃变量分析时，变量的数量超过容量 %d"), MAX_NODE);
			throw Exception(s.ToString());
		}
		if (!defs.Contains(index))  // 使用前没有定值
		{
			state += index;
		}
		uses += index;  // 标记使用
	}
}


// 首先计算出每个基本块的引用集和定义集
void LiveVariableAnalysis::Initialize()
{
	for (auto block : GetFunction()->GetBasicBlocks())
	{
		NodeSet defs = 0;  // 前3位表示 AXY 是否定义
		NodeSet uses = 0;  // 前3位表示 AXY 是否被使用
		auto blockSet = allocator.New<BasicBlockLiveVariableSet>();
		block->tag = blockSet;
		for (auto tac : block->GetCodes())
		{
			// 函数调用也可能给AXY定值
			if (tac->op == TACOperator::CALL)
			{
				auto sub = db.FindSubroutine(tac->z.GetValue());
				if (sub)  // 找不到说明还没分析，先不管
				{
					if (sub->flag & SUBF_PARAM)
					{
						if ((defs & sub->flag) == 0)  // 使用前没有定值
						{
							blockSet->uses |= NodeSet(sub->flag & SUBF_PARAM);
						}
						uses |= NodeSet(sub->flag & SUBF_PARAM);  // 标记使用
					}
					auto rets = (sub->flag & SUBF_RETURN) >> 3;
					if (rets)
					{
						if ((uses & NodeSet(rets)) == 0)  // 定值前没有使用
						{
							blockSet->defs |= NodeSet(rets);
						}
						defs |= NodeSet(rets);  // 标记定值
					}
					continue;
				}
			}
			else if (tac->op == TACOperator::ARRAY_SET)
			{
				// 数组元素赋值：x[y] = z，使用 y，z，x比不可能是AXY寄存器，不管
				AnalyzeReference(tac->y, defs, uses, blockSet->uses);
				AnalyzeReference(tac->z, defs, uses, blockSet->uses);
				continue;
			}
			// 通常情况：z = x op y，定义 z，使用 x，y
			AnalyzeReference(tac->x, defs, uses, blockSet->uses);
			AnalyzeReference(tac->y, defs, uses, blockSet->uses);
			AnalyzeReference(tac->z, uses, defs, blockSet->defs);
		}
		/*Sprintf<> s;
		s.Append(_T("基本块%04X，使用: "), block->GetStartAddress());
		FormatVariableSet(s, blockSet->uses);
		s.Append(_T(", 定义: "));
		FormatVariableSet(s, blockSet->defs);
		s.Append(_T("\n"));
		COUT << s.ToString();*/
	}
}

bool LiveVariableAnalysis::AnalyzeNode(TACBasicBlock* block)
{
	auto blockSet = (BasicBlockLiveVariableSet*)block->tag;
	// OUT[B] = 所有后继活跃变量的并集
	// IN[B] = useB 并 (OUT[B] - defB)
	for (TACBasicBlock* next : block->nexts)
	{
		auto nextSet = (BasicBlockLiveVariableSet*)next->tag;
		blockSet->out |= nextSet->in;
	}
	auto value = blockSet->in;
	blockSet->in = (blockSet->out & ~blockSet->defs) | blockSet->uses;
	//DumpAllBasicBlockLiveVariables(subroutine->GetBasicBlocks());
	//COUT << std::endl;
	return blockSet->in == value;
}
