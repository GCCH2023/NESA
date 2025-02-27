#include "stdafx.h"
#include "TACPeephole.h"
#include "TAC.h"

enum class TACValueKind
{
	Undefined,  // 未定义
	Constant,  // 常量
	Unknown,  // 不确定
};

struct TACValue
{
	TACValueKind kind;
	TACOperand value;  // 如果是常量，则使用这个字段保存它的值
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
	// 遍历基本块
	for (auto block : subroutine->GetBasicBlocks())
	{
		memset(axyValue, 0, sizeof(axyValue));
		last = nullptr;  // 只在一个基本块内进行优化
		for (auto tac : block->GetCodes())
		{
			// 1. 首先，尝试用常量替换操作数 x 和 y
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
			// 2. 接着尝试常量折叠
			try
			{
				tac->x = Evaluate(tac->op, tac->x, tac->y);
				tac->y = 0;
			}
			catch (Exception&)
			{
				// 不能折叠就算了
			}
			// 3. 尝试 赋值替换 a = b, c = a, 替换为 c = b
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

			// 最后更新寄存器的值
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
