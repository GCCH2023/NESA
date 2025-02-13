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
	Sprintf<> s;
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

// 只分析寄存器 A, X, Y，其他寄存器和临时变量或者全局变量忽略掉
//void LiveVariableAnalysis::Analyze(TACSubroutine* subroutine)
//{
//	if (!subroutine)
//		return;
//
//	this->subroutine = subroutine;
//
//	Initialize();
//
//	bool isEnd = false;
//	// 迭代
//	int iter = 0;
//	while (!isEnd)
//	{
//		isEnd = true;
//		//s.Append(_T("迭代 %d\n", iter++);
//		bool isEntry = true;
//		// 遍历基本块
//		for (auto block : subroutine->GetBasicBlocks())
//		{
//			auto blockSet = (BasicBlockLiveVariableSet*)block->tag;
//			// OUT[B] = 所有后继活跃变量的并集
//			// IN[B] = useB 并 (OUT[B] - defB)
//			for (TACBasicBlock* next : block->nexts)
//			{
//				auto nextSet = (BasicBlockLiveVariableSet*)next->tag;
//				blockSet->out |= nextSet->in;
//			}
//			auto value = blockSet->in;
//			blockSet->in = (blockSet->out & ~blockSet->defs) | blockSet->uses;
//			if (blockSet->in != value)
//				isEnd = false;
//		}
//	}
//
//	// DumpAllBasicBlockLiveVariables(subroutine->GetBasicBlocks());
//}

// 分析寄存器 AXY 的引用（定义或使用）情况
void AnalyzeAXYOperandReference(TACOperand& operand, BitSet32& defs, BitSet32& uses, BitSet32& state)
{
	if (operand.IsRegister())
	{
		int index = operand.GetValue();
		if (index <= Nes::NesRegisters::Y)
		{
			if ((defs & (1 << index)) == 0)  // 使用前没有定值
			{
				state |= (1 << index);
			}
			uses |= 1 << index;  // 标记使用
		}
	}
}


// 首先计算出每个基本块的引用集和定义集
void LiveVariableAnalysis::Initialize()
{
	for (auto block : this->subroutine->GetBasicBlocks())
	{
		BitSet32 defs = 0;  // 前3位表示 AXY 是否定义
		BitSet32 uses = 0;  // 前3位表示 AXY 是否被使用
		auto blockSet = allocator.New<BasicBlockLiveVariableSet>();
		for (auto tac : block->GetCodes())
		{
			// 函数调用也可能给AXY定值
			if (tac->op == TACOperator::CALL)
			{
				auto sub = db.FindSubroutine(tac->z.GetValue());
				if (sub->flag & SUBF_PARAM)
				{
					if ((defs & sub->flag) == 0)  // 使用前没有定值
					{
						blockSet->uses |= sub->flag & SUBF_PARAM;
					}
					uses |= sub->flag & SUBF_PARAM;  // 标记使用
				}
				auto rets = (sub->flag & SUBF_RETURN) >> 3;
				if (rets)
				{
					if ((uses & rets) == 0)  // 定值前没有使用
					{
						blockSet->defs |= rets;
					}
					defs |= rets;  // 标记定值
				}
				continue;
			}
			AnalyzeAXYOperandReference(tac->x, defs, uses, blockSet->uses);
			AnalyzeAXYOperandReference(tac->y, defs, uses, blockSet->uses);
			AnalyzeAXYOperandReference(tac->z, uses, defs, blockSet->defs);
			block->tag = blockSet;
		}
		/*s.Append(_T("基本块%04X，使用: ", block->GetStartAddress());
		if (blockSet->uses & 1)
			s.Append(_T("A, ");
		if (blockSet->uses & 2)
			s.Append(_T("X, ");
		if (blockSet->uses & 4)
			s.Append(_T("Y, ");
		s.Append(_T(", 定义: ");
		if (blockSet->defs & 1)
			s.Append(_T("A, ");
		if (blockSet->defs & 2)
			s.Append(_T("X, ");
		if (blockSet->defs & 4)
			s.Append(_T("Y, ");
		s.Append(_T("\n");*/
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
