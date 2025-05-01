#include "stdafx.h"
#include "CASTDoWhileOptimier.h"
#include "CNode.h"
#include "VariableUsed.h"
#include "NodeConverter.h"

// 检查双目表达式的两个操作数是否满足：一个是变量，另一个是整数
bool MatchVariableInteger(const Expression* node)
{
	// x op y，x，y中一个是变量，一个是整数
	// op 是关系运算符
	constexpr auto mask = GetCategory(CNodeKind::EXPR_INTEGER) | GetCategory(CNodeKind::EXPR_VARIABLE);
	const uint32_t value = GetCategory(node->GetLeftOperand()->GetKind()) | GetCategory(node->GetRightOperand()->GetKind());
	return value == mask;
}

// 检查条件表达式是否满足：变量和整数比较
bool MatchCompare(const Expression* node)
{
	return MatchVariableInteger(node) && node->IsCompare();
}


CASTDoWhileOptimier::CASTDoWhileOptimier():
	parents(16)
{
	parents.clear();
}

void CASTDoWhileOptimier::Reset()
{
}

// 判断指定节点是否使用了指定变量
bool UsedVariable(CNode* node, Expression* variable)
{
	VariableUsed vu(variable->GetVariable());
	vu.Visit(node);
	return vu.IsUsed();
}

// 获取变量的整数赋值语句，失败返回空
Statement* GetDefinedStatement(Statement* node, Expression* variable)
{
	if (!node->IsExprStatement())
		return nullptr;

	auto expr = node->GetExpression();
	// 要求是 变量 = 整数 这样的形式
	if (!expr->IsAssign())
		return nullptr;

	auto left = expr->GetLeftOperand();
	auto right = expr->GetRightOperand();
	if (!MatchVariableInteger(expr))
		return nullptr;

	if (left->GetVariable() == variable->GetVariable())
		return node;
	return nullptr;
}

// 获取变量的赋值语句，失败返回空
Statement* GetVariableAssignment(Statement* node, Expression* variable)
{
	if (!node->IsExprStatement())
		return nullptr;

	auto expr = node->GetExpression();
	// 要求是 变量 = 整数 这样的形式
	if (expr->GetKind() != CNodeKind::EXPR_ASSIGN || !expr->GetLeftOperand()->IsVariable())
		return nullptr;

	if (expr->GetLeftOperand()->GetVariable() == variable->GetVariable())
		return node;
	return nullptr;
}


// 查找do while 语句 node 的初始化语句，var 是迭代变量
Statement* CASTDoWhileOptimier::GetInitializeStatement(Statement* node, Expression* var)
{
	Statement* init = nullptr;
	for (Statement* prev = node->GetPrev(); prev; prev = prev->GetPrev())
	{
		// 判断是否定值语句
		if ((init = GetDefinedStatement(prev, var)) != nullptr)
			break;
		// 如果使用了迭代变量，就认为失败
		if (UsedVariable(prev, var))
			return nullptr;
	}
	return init;
}

// 查找do while 语句 node 的迭代语句，var 是迭代变量
Statement* CASTDoWhileOptimier::GetIteratorStatement(Statement* node, Expression* var)
{
	Statement* iter = nullptr;
	auto body = node->GetLoopBody();
	if (!body->IsCompound())
		return nullptr;

	auto& list = body->AsList();
	for (auto it = list.rbegin(); it != list.rend(); ++it)
	{
		// 判断是否定值语句
		if ((iter = GetVariableAssignment(*it, var)) != nullptr)
			break;
		// 如果使用了迭代变量，就认为失败
		if (UsedVariable(*it, var))
			return nullptr;
	}
	return iter;
}

void CASTDoWhileOptimier::OptimizeDoWhile(Statement* node)
{
	// (1) 判断条件表达式
	if (!MatchCompare(node->GetLoopCondition()))
		return;

	auto cond = node->GetLoopCondition();
	Expression* var, * value;
	if (cond->GetLeftOperand()->IsVariable())
	{
		var = cond->GetLeftOperand();
		value = cond->GetRightOperand();
	}
	else
	{
		value = cond->GetLeftOperand();
		var = cond->GetRightOperand();
	}

	// (2) 查找初始化语句
	Statement* init = GetInitializeStatement(node, var);
	if (!init)
		return;

	// (3) 查找迭代语句
	auto body = node->GetLoopBody();
	Statement* iter = GetIteratorStatement(node, var);
	if (!iter)
		return;

	// (4) 保证for循环至少执行一次
	auto initValue = init->GetExpression()->GetRightOperand();  // 赋值语句的表达式的y操作数
	if (!Evaluate(cond->GetKind(), *initValue, *value))
		return;

	// 可以转换为 for 语句
	// (1) 移除初始化语句
	// 必有父节点且是复合语句
	auto list = static_cast<Statement*>(GetParent());
	list->AsList().erase(init);
	// (2) 移除迭代语句
	body->AsList().erase(iter);
	// 修改 do while 为 for
	NodeConverter::To(node, Statement::For(body, init->GetExpression(), node->GetLoopCondition(), iter->GetExpression()));
}

void CASTDoWhileOptimier::OnVisit(Statement* node)
{
	parents.push_back(node);

	VisitChildren(node);

	parents.pop_back();

	if (node->GetKind() != CNodeKind::STAT_DO_WHILE)
		return;

	OptimizeDoWhile(node);
}

void CASTDoWhileOptimier::OnVisit(Expression* node)
{
	// 不需要遍历表达式

}

