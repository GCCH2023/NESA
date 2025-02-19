#include "stdafx.h"
#include "TACPeephole.h"
#include "TAC.h"

enum class TACValueKind
{
	Undefined,  // δ����
	Constant,  // ����
	Unknown,  // ��ȷ��
};

struct TACValue
{
	TACValueKind kind;
	TACOperand value;  // ����ǳ�������ʹ������ֶα�������ֵ
};

TACPeephole::TACPeephole(NesDataBase& db):
TACOptimizer(db)
{
}


TACPeephole::~TACPeephole()
{
}

void TACPeephole::Optimize(TACSubroutine* subroutine)
{
	TACValue axyValue[3];
	// ����������
	for (auto block : subroutine->GetBasicBlocks())
	{
		memset(axyValue, 0, sizeof(axyValue));
		for (auto tac : block->GetCodes())
		{
			// ���ȣ������ó����滻������ x �� y
			if (tac->x.IsRegister() && tac->x.GetValue() <= Nes::NesRegisters::Y)
			{
				int index = tac->x.GetValue();
				if (axyValue[index].kind == TACValueKind::Constant)
					tac->x = axyValue[index].value;
			}
			if (tac->y.IsRegister() && tac->y.GetValue() <= Nes::NesRegisters::Y)
			{
				int index = tac->y.GetValue();
				if (axyValue[index].kind == TACValueKind::Constant)
					tac->y = axyValue[index].value;
			}
			// ���ų��Գ����۵�
			try
			{
				tac->x = Evaluate(tac->op, tac->x, tac->y);
				tac->y = 0;
			}
			catch (Exception&)
			{
				// �����۵�������
			}
			// �����¼Ĵ�����ֵ
			if (tac->z.IsRegister() && tac->z.GetValue() <= Nes::NesRegisters::Y)
			{
				if (tac->x.IsInterger())
				{
					axyValue[tac->z.GetValue()] = { TACValueKind::Constant, tac->x };
				}
				else
				{
					axyValue[tac->z.GetValue()] = { TACValueKind::Unknown, tac->x };
				}
			}
		}
	}
}
