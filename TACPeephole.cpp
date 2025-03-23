#include "stdafx.h"
#include "TACPeephole.h"
#include "TACFunction.h"

TACPeephole::TACPeephole(NesDataBase& db):
TACOptimizer(db)
{
}


TACPeephole::~TACPeephole()
{
}

// 将布尔表达式合并到 IFTRUE 或 IFFALSE 分支，返回是否合并
bool CombineConditionalBranch(TAC* tac, TAC* boolExpr)
{
	if (tac->op == TACOperator::IFTRUE)
	{
		switch (boolExpr->op)
		{
		case TACOperator::BOOL_GREAT:  // z = x > y => if x > y goto z
			tac->op = TACOperator::IFGREAT;
			break;
		case TACOperator::BOOL_GEQ:  // z = x >= y => if x >= y goto z
			tac->op = TACOperator::IFGEQ;
			break;
		case TACOperator::BOOL_LESS:  // z = x < y => if x < y goto z
			tac->op = TACOperator::IFLESS;
			break;
		case TACOperator::BOOL_LEQ:  // z = x <= y => if x <= y goto z
			tac->op = TACOperator::IFLEQ;
			break;
		case TACOperator::BOOL_EQ:  // z = x == y => if x == y goto z
			tac->op = TACOperator::IFEQ;
			break;
		case TACOperator::BOOL_NEQ:  // z = x != y => if x != y goto z
			tac->op = TACOperator::IFNEQ;
			break;
		default: return false;
		}
	}
	else if (tac->op == TACOperator::IFFALSE)
	{
		switch (boolExpr->op)
		{
		case TACOperator::BOOL_GREAT:  // z = x > y => if x <= y goto z
			tac->op = TACOperator::IFLEQ;
			break;
		case TACOperator::BOOL_GEQ:  // z = x >= y => if x < y goto z
			tac->op = TACOperator::IFLESS;
			break;
		case TACOperator::BOOL_LESS:  // z = x < y => if x >= y goto z
			tac->op = TACOperator::IFGEQ;
			break;
		case TACOperator::BOOL_LEQ:  // z = x <= y => if x > y goto z
			tac->op = TACOperator::IFGREAT;
			break;
		case TACOperator::BOOL_EQ:  // z = x == y => if x != y goto z
			// 如果有一个操作数是 0，还可以优化
			if (boolExpr->x.IsZero())
			{
				// a = y1 == 0, if! a goto L => if y1 goto L
				tac->op = TACOperator::IFTRUE;
				tac->x = boolExpr->y;
				tac->y = 0;
				return true;
			}
			else if (boolExpr->y.IsZero())
			{
				// a = x1 == 0, if! a goto L => if x1 goto L
				tac->op = TACOperator::IFTRUE;
				tac->x = boolExpr->x;
				tac->y = 0;
				return true;
			}
			tac->op = TACOperator::IFNEQ;
			break;
		case TACOperator::BOOL_NEQ:  // z = x != y => if x == y goto z
			if (boolExpr->x.IsZero())
			{
				// z = x1 == 0 => if! x1 goto z
				tac->op = TACOperator::IFFALSE;
				tac->x = boolExpr->y;
				tac->y = 0;
				return true;
			}
			else if (boolExpr->y.IsZero())
			{
				// z = y1 == 0 => if! y1 goto z
				tac->op = TACOperator::IFFALSE;
				tac->x = boolExpr->x;
				tac->y = 0;
				return true;
			}
			tac->op = TACOperator::IFEQ;
			break;
		default: return false;
		}
	}
	else
	{
		return false;
	}
	tac->x = boolExpr->x;
	tac->y = boolExpr->y;
	return true;
}

// 获取操作数的定值指令
// 要求操作数是寄存器或临时变量
// 如果操作数在其他基本块定值，则返回nullptr
TAC* GetOperandDefinition(std::vector<TAC*>& varValues, TACOperand& operand)
{
	int index = GetAxyNvzcTempIndex(operand);
	return varValues[index];
}

// 设置操作数的定值指令
void SetOperandDefinition(std::vector<TAC*>& varValues, TAC* tac)
{
	if (IsAxyNvzcTemp(tac->z) && tac->op != TACOperator::ARRAY_SET)
	{
		int index = GetAxyNvzcTempIndex(tac->z);
		varValues[index] = tac;
	}
}

// 判断操作数的值是否发生改变
bool IsOperandChanged(std::vector<TAC*>& varValues, TACOperand& operand, TAC* current)
{
	// 如果有定值点，并且定值点地址大于等于使用点地址，说明改变了
	auto tac = GetOperandDefinition(varValues, operand);
	return tac && tac->address >= current->address;
}

// 进行代数优化
void OptimizeExpression(TACOperand& operand, TACOperand& other, TAC* current, TAC* tac, std::vector<TAC*>& varValues)
{
	// a = b op1 num1, c = a op2 num2 =>
	// c = (b op1 num1) op2 num2 =>
	// c = b op3 num3
	// op3 由 op1 和 op2 决定
	if (!IsAxyNvzcTemp(tac->x) || IsOperandChanged(varValues, tac->x, current))
		return;

	if (CombineConditionalBranch(current, tac))
		return;

	if (!tac->y.IsInterger())
		return;

	switch (tac->op)
	{
	case TACOperator::SUB:  // a = b - num1
		switch (current->op)
		{
			// d = a < num2 => d = b - num1 < num2 => b < num1 + num2
		case TACOperator::BOOL_LESS:
		case TACOperator::IFLESS:
			operand = tac->x;
			other = TACOperand(tac->y.GetValue() + other.GetValue());
			break;
			// d = a <= num2 => d = b - num1 <= num2 => b <= num1 + num2
		case TACOperator::BOOL_LEQ:
		case TACOperator::IFLEQ:
			operand = tac->x;
			other = TACOperand(tac->y.GetValue() + other.GetValue());
			break;
			// d = a == num2 => d = b - num1 == num2 => b == num1 + num2
		case TACOperator::BOOL_EQ:
		case TACOperator::IFEQ:
			operand = tac->x;
			other = TACOperand(tac->y.GetValue() + other.GetValue());
			break;
		}
		break;
	case TACOperator::BOOL_EQ:  // a = b == num1
		switch (current->op)
		{
			// d = a == num2 => d = (b == num1) == num2
		case TACOperator::BOOL_LEQ:
		case TACOperator::IFLEQ:
			if (tac->y.GetValue() == other.GetValue())  // num1 == num2 => d = b == num1
				operand = tac->x;
			break;
			// if a goto z => if b == num1 goto z
		case TACOperator::IFTRUE:
			current->x = tac->x;
			current->y = tac->y;
			current->op = TACOperator::IFEQ;
			break;
			// if !a goto z => if b != num1 goto z
		case TACOperator::IFFALSE:
			current->x = tac->x;
			current->y = tac->y;
			current->op = TACOperator::IFNEQ;
			break;
		}
		break;
	case TACOperator::BOOL_LESS:  // a = b < num1
		switch (current->op)
		{
			// if a goto z => if b < num1 goto z
		case TACOperator::IFTRUE:
			current->x = tac->x;
			current->y = tac->y;
			current->op = TACOperator::IFEQ;
			break;
		case TACOperator::IFFALSE:
			current->x = tac->x;
			current->y = tac->y;
			current->op = TACOperator::IFNEQ;
			break;
		}
		break;
	}
}

// 尝试用常量替换操作数
// operand : 当前处理的操作数
// other : 三地址码中的另一个操作数
void TryReplaceOperand(TACOperand& operand, TACOperand& other, TAC* current, std::vector<TAC*>& varValues)
{
	if (IsAxyNvzcTemp(operand))
	{
		auto tac = GetOperandDefinition(varValues, operand);
		if (!tac)
			return;  // 在其他基本块定值
		if (tac->op == TACOperator::ASSIGN)  // 首先处理赋值指令
		{
			if (tac->x.IsInterger())  // 用常量赋值，则替换为常量
			{
				operand = tac->x;
			}
			else if(IsAxyNvzcTemp(tac->x))
			{
				// 寄存器或临时变量，只要用于赋值的变量的值没变，也可以替换
				if (!IsOperandChanged(varValues, tac->x, tac))
				{
					// 在其他基本块定值或者在当前指令之前定值
					operand = tac->x;
				}
			}
			//else if (tac->x.IsGlobal())
			//{
			//	// 全局变量，如果是上一条指令，就替换
			//	if (tac->address + )
			//}

		}
		else if (other.IsInterger())  // 尝试代数优化
		{
			OptimizeExpression(operand, other, current, tac, varValues);
		}
	}
}

// 尝试优化条件分支 IFTRUE 和 IFFALSE
void TryOptimizeIf(TAC* tac, std::vector<TAC*>& varValues)
{
	if (IsAxyNvzcTemp(tac->x))
	{
		auto def = GetOperandDefinition(varValues, tac->x);
		if (def && IsBool(def->op))
		{
			CombineConditionalBranch(tac, def);
		}
	}
}

void TACPeephole::Optimize(TACFunction* subroutine)
{
	Reset();

	// 遍历基本块
	for (auto block : subroutine->GetBasicBlocks())
	{
		std::vector<TAC*> varValues(256);  // 每个变量的定值指令

		for (auto tac : block->GetCodes())
		{
			// 1. 首先，尝试用常量替换操作数 x 和 y
			TryReplaceOperand(tac->x, tac->y, tac, varValues);
			TryReplaceOperand(tac->y, tac->x, tac, varValues);
			// 2. 接着尝试常量折叠
			try
			{
				tac->x = Evaluate(tac->op, tac->x, tac->y);
				tac->y = 0;
				tac->op = TACOperator::ASSIGN;
			}
			catch (Exception&)
			{
				// 不能折叠就算了
			}

			//if (tac->op == TACOperator::IFTRUE || tac->op == TACOperator::IFFALSE)
			//{
			//	// 尝试合并布尔表达式
			//	TryOptimizeIf(tac, varValues);
			//}
			SetOperandDefinition(varValues, tac);
		}
	}
}

void TACPeephole::Reset()
{

}
