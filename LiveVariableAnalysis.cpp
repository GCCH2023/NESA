#include "stdafx.h"
#include "LiveVariableAnalysis.h"
#include "TAC.h"
#include "NesDataBase.h"
using namespace std;
#include "TACTranslater.h"


LiveVariableAnalysis::LiveVariableAnalysis(NesDataBase& db_, Allocator& allocator_):
DataFlowAnalyzer(db_),
allocator(allocator_)
{

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
			if (blockSet->in.Contains(Nes::NesRegisters::A))
				s.Append(_T("A, "));
			if (blockSet->in.Contains(Nes::NesRegisters::X))
				s.Append(_T("X, "));
			if (blockSet->in.Contains(Nes::NesRegisters::Y))
				s.Append(_T("Y, "));
		}
		s.Append(_T(" OUT: "));
		if (blockSet->out.None())
		{
			s.Append(_T("无 "));
		}
		else
		{
			if (blockSet->out.Contains(Nes::NesRegisters::A))
				s.Append(_T("A, "));
			if (blockSet->out.Contains(Nes::NesRegisters::X))
				s.Append(_T("X, "));
			if (blockSet->out.Contains(Nes::NesRegisters::Y))
				s.Append(_T("Y, "));
		}
		s.Append(_T("\n"));
	}
	COUT << s.ToString();
}

// 分析寄存器 AXY 的引用（定义或使用）情况
void AnalyzeAXYOperandReference(TACOperand& operand, NodeSet& defs, NodeSet& uses, NodeSet& state)
{
	if (operand.IsRegister())
	{
		int index = operand.GetValue();
		if (index <= Nes::NesRegisters::Y)
		{
			if (!defs.Contains(index))  // 使用前没有定值
			{
				state += index;
			}
			uses += index;  // 标记使用
		}
	}
}


// 首先计算出每个基本块的引用集和定义集
void LiveVariableAnalysis::Initialize()
{
	for (auto block : this->subroutine->GetBasicBlocks())
	{
		NodeSet defs = 0;  // 前3位表示 AXY 是否定义
		NodeSet uses = 0;  // 前3位表示 AXY 是否被使用
		auto blockSet = allocator.New<BasicBlockLiveVariableSet>();
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
			AnalyzeAXYOperandReference(tac->x, defs, uses, blockSet->uses);
			AnalyzeAXYOperandReference(tac->y, defs, uses, blockSet->uses);
			AnalyzeAXYOperandReference(tac->z, uses, defs, blockSet->defs);
			block->tag = blockSet;
		}
		/*Sprintf<> s;
		s.Append(_T("基本块%04X，使用: "), block->GetStartAddress());
		if (blockSet->uses.Contains(Nes::NesRegisters::A))
			s.Append(_T("A, "));
		if (blockSet->uses.Contains(Nes::NesRegisters::X))
			s.Append(_T("X, "));
		if (blockSet->uses.Contains(Nes::NesRegisters::Y))
			s.Append(_T("Y, "));
		s.Append(_T(", 定义: "));
		if (blockSet->defs.Contains(Nes::NesRegisters::A))
			s.Append(_T("A, "));
		if (blockSet->defs.Contains(Nes::NesRegisters::X))
			s.Append(_T("X, "));
		if (blockSet->defs.Contains(Nes::NesRegisters::Y))
			s.Append(_T("Y, "));
		s.Append(_T("\n"));
		COUT << s.ToString();*/
	}
}

bool LiveVariableAnalysis::IteraterBasicBlock(TACBasicBlock* block)
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
	return blockSet->in == value;
}
