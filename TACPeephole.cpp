#include "stdafx.h"
#include "TACPeephole.h"
#include "ReachingDefinition.h"


// 操作数是否是要分析的变量
bool IsVariable(const TACOperand& operand)
{
	switch (operand.GetKind())
	{
	case TACOperand::TEMP: return true;
	case TACOperand::GLOBAL: return true;
	case TACOperand::REGISTER: return operand.GetValue() <= TAC_REG_C;
	default: return false;
	}
}



TACPeephole::TACPeephole(NesDataBase& db, std::shared_ptr<ReachingDefinitionResult> rdResult):
TACOptimizer(db),
reachDefResult(rdResult)
{
}

TACPeephole::~TACPeephole()
{
}

// 设置操作数的定值指令
void TACPeephole::SetOperandDefinition(TAC* tac)
{
	if (tac->op == TACOperator::ARRAY_SET)
		return;
	if (tac->op == TACOperator::CALL)
	{
		// 函数调用可能会对全局变量赋值，删除所有全局变量的定值点
		auto it = varDefMap.begin();
		while (it != varDefMap.end())
		{
			if (it->first.IsGlobal())
				it = varDefMap.erase(it);
			else
				++it;
		}
	}
	if (IsVariable(tac->z))
		varDefMap[tac->z] = tac;
}


TAC* TACPeephole::GetOperandDefinition(TACOperand& operand)
{
	// 当前基本块有定值，就返回它
	auto tac = varDefMap[operand];
	if (tac)
		return tac;
	// 否则返回可以到达的定值
	if (reachDefResult)
	{
		TACList tacList;
		auto index = (size_t)GetCurrentBasicBlock()->tag;
		reachDefResult->GetBasicBlockDefinitionsIn(tacList, index, operand);
		if (tacList.size() == 1)
			return tacList.front();
	}
	return nullptr;
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

// 判断操作数的值是否发生改变
bool TACPeephole::IsOperandChanged(TACOperand& operand, TAC* current)
{
	// 如果有定值点，并且定值点地址大于等于使用点地址，说明改变了
	auto tac = GetOperandDefinition(operand);
	return tac && tac->address >= current->address;
}

// 进行代数优化
void TACPeephole::OptimizeExpression(TACOperand& operand, TACOperand& other, TAC* current, TAC* tac)
{
	// a = b op1 num1, c = a op2 num2 =>
	// c = (b op1 num1) op2 num2 =>
	// c = b op3 num3
	// op3 由 op1 和 op2 决定
	if (!IsVariable(tac->x) || IsOperandChanged(tac->x, current))
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
// operand : 要处理的操作数
void TACPeephole::TryReplaceOperand(TACOperand& operand)
{
	if (!IsAxyNvzcTemp(operand))
		return;

	auto tac = GetOperandDefinition(operand);
	if (!tac)
		return;  // 在其他基本块定值
	if (tac->op == TACOperator::ASSIGN)  // 首先处理赋值指令
	{
		if (tac->x.IsInterger())  // 用常量赋值，则替换为常量
		{
			operand = tac->x;
		}
		else if (IsVariable(tac->x))
		{
			// 寄存器或临时变量，只要用于赋值的变量的值没变，也可以替换
			if (!IsOperandChanged(tac->x, tac))
			{
				// 在其他基本块定值或者在当前指令之前定值
				operand = tac->x;
			}
		}
	}
}


// 尝试用常量替换操作数并进行代数优化
// operand : 当前处理的操作数
// other : 三地址码中的另一个操作数
// current : 当前优化的指令
void TACPeephole::TryReplaceOperand(TACOperand& operand, TACOperand& other, TAC* current)
{
	if (!IsAxyNvzcTemp(operand))
		return;

	auto tac = GetOperandDefinition(operand);
	if (!tac)
		return;  // 在其他基本块定值
	if (tac->op == TACOperator::ASSIGN)  // 首先处理赋值指令
	{
		if (tac->x.IsInterger())  // 用常量赋值，则替换为常量
		{
			operand = tac->x;
		}
		else if (IsVariable(tac->x))
		{
			// 寄存器或临时变量，只要用于赋值的变量的值没变，也可以替换
			if (!IsOperandChanged(tac->x, tac))
			{
				// 在其他基本块定值或者在当前指令之前定值
				operand = tac->x;
			}
		}
	}
	else if (other.IsInterger())  // 尝试代数优化
	{
		OptimizeExpression(operand, other, current, tac);
	}
}

void TACPeephole::Optimize(TACFunction* subroutine)
{
	Reset();

	// 遍历基本块
	for (auto block : subroutine->GetBasicBlocks())
	{
		currentBlock = block;
		varDefMap.clear();  // 不能跨基本块
		for (auto tac : block->GetCodes())
		{
			// 数组赋值特殊处理
			if (tac->op == TACOperator::ARRAY_SET)
			{
				TryReplaceOperand(tac->z);
				continue;
			}
			// 1. 首先，尝试用常量替换操作数 x 和 y
			TryReplaceOperand(tac->x, tac->y, tac);
			TryReplaceOperand(tac->y, tac->x, tac);
			// 2. 接着尝试常量折叠，直接计算出两个常量的结果
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

			SetOperandDefinition(tac);
		}
	}
}

void TACPeephole::Reset()
{
}
