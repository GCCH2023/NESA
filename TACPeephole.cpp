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
	Reset();

	TACValue axyValue[3];
	TAC* last = nullptr;
	// ����������
	for (auto block : subroutine->GetBasicBlocks())
	{
		memset(axyValue, 0, sizeof(axyValue));
		last = nullptr;  // ֻ��һ���������ڽ����Ż�
		for (auto tac : block->GetCodes())
		{
			// 1. ���ȣ������ó����滻������ x �� y
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
			// 2. ���ų��Գ����۵�
			try
			{
				tac->x = Evaluate(tac->op, tac->x, tac->y);
				tac->y = 0;
			}
			catch (Exception&)
			{
				// �����۵�������
			}
			// 3. ���� ��ֵ�滻 a = b, c = a, �滻Ϊ c = b
			if (last && last->op == TACOperator::ASSIGN)
			{
				if (tac->x == last->z)
				{
					tac->x = last->x;
				}
				if (tac->y == last->z)
				{
					tac->y = last->x;
				}
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

			last = tac;
		}
	}
}

void TACPeephole::Reset()
{

}
