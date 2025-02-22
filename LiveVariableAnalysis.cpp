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

// ���ÿ�����������ڻ�Ծ�������ͳ��ڻ�Ծ������
void DumpAllBasicBlockLiveVariables(TACBasicBlockList& blocks)
{
	Sprintf<512> s;
	s.Format(_T("������Ļ�Ծ��������\n"));
	for (auto block : blocks)
	{
		s.Append(_T("������ %04X IN : "), block->GetStartAddress());
		auto blockSet = (BasicBlockLiveVariableSet*)block->tag;
		if (blockSet->in.None())
		{
			s.Append(_T("�� "));
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
			s.Append(_T("�� "));
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

// �����Ĵ��� AXY �����ã������ʹ�ã����
void AnalyzeAXYOperandReference(TACOperand& operand, NodeSet& defs, NodeSet& uses, NodeSet& state)
{
	if (operand.IsRegister())
	{
		int index = operand.GetValue();
		if (index <= Nes::NesRegisters::Y)
		{
			if (!defs.Contains(index))  // ʹ��ǰû�ж�ֵ
			{
				state += index;
			}
			uses += index;  // ���ʹ��
		}
	}
}


// ���ȼ����ÿ������������ü��Ͷ��弯
void LiveVariableAnalysis::Initialize()
{
	for (auto block : this->subroutine->GetBasicBlocks())
	{
		NodeSet defs = 0;  // ǰ3λ��ʾ AXY �Ƿ���
		NodeSet uses = 0;  // ǰ3λ��ʾ AXY �Ƿ�ʹ��
		auto blockSet = allocator.New<BasicBlockLiveVariableSet>();
		for (auto tac : block->GetCodes())
		{
			// ��������Ҳ���ܸ�AXY��ֵ
			if (tac->op == TACOperator::CALL)
			{
				auto sub = db.FindSubroutine(tac->z.GetValue());
				if (sub)  // �Ҳ���˵����û�������Ȳ���
				{
					if (sub->flag & SUBF_PARAM)
					{
						if ((defs & sub->flag) == 0)  // ʹ��ǰû�ж�ֵ
						{
							blockSet->uses |= NodeSet(sub->flag & SUBF_PARAM);
						}
						uses |= NodeSet(sub->flag & SUBF_PARAM);  // ���ʹ��
					}
					auto rets = (sub->flag & SUBF_RETURN) >> 3;
					if (rets)
					{
						if ((uses & NodeSet(rets)) == 0)  // ��ֵǰû��ʹ��
						{
							blockSet->defs |= NodeSet(rets);
						}
						defs |= NodeSet(rets);  // ��Ƕ�ֵ
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
		s.Append(_T("������%04X��ʹ��: "), block->GetStartAddress());
		if (blockSet->uses.Contains(Nes::NesRegisters::A))
			s.Append(_T("A, "));
		if (blockSet->uses.Contains(Nes::NesRegisters::X))
			s.Append(_T("X, "));
		if (blockSet->uses.Contains(Nes::NesRegisters::Y))
			s.Append(_T("Y, "));
		s.Append(_T(", ����: "));
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
	// OUT[B] = ���к�̻�Ծ�����Ĳ���
	// IN[B] = useB �� (OUT[B] - defB)
	for (TACBasicBlock* next : block->nexts)
	{
		auto nextSet = (BasicBlockLiveVariableSet*)next->tag;
		blockSet->out |= nextSet->in;
	}
	auto value = blockSet->in;
	blockSet->in = (blockSet->out & ~blockSet->defs) | blockSet->uses;
	return blockSet->in == value;
}
