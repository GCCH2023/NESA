#include "stdafx.h"
#include "CASTDoWhileOptimier.h"
#include "CNode.h"

// 检查双目表达式的两个操作数是否满足：一个是变量，另一个是整数
bool MatchVariableInteger(const CNode* node)
{
	// x op y，x，y中一个是变量，一个是整数
	// op 是关系运算符
	constexpr auto mask = GetCategory(CNodeKind::EXPR_INTEGER) | GetCategory(CNodeKind::EXPR_VARIABLE);
	const uint32_t value = GetCategory(node->e.x->kind) | GetCategory(node->e.y->kind);
	return value == mask;
}

// 检查条件表达式是否满足：变量和整数比较
bool MatchCompare(const CNode* node)
{
	return MatchVariableInteger(node) && node->IsCompareExpression();
}


void CASTDoWhileOptimier::Reset()
{
}

// 判断指定节点是否使用了指定变量
bool UsedVariable(CNode* node, CNode* variable)
{
	if (!node)
		return false;
	switch (node->kind)
	{
	case CNodeKind::EXPR_VARIABLE:
		return node->variable->name == variable->variable->name;
	case CNodeKind::EXPR_INTEGER:
		return false;
	case CNodeKind::STAT_EMPTY:
		return false;
	case CNodeKind::STAT_GOTO:
		return true;  // 不知道有没有使用，当作使用处理
	case CNodeKind::STAT_LIST:
		for (auto child : CListNode(*node))
		{
			if (child == variable)
				return true;
		}
		return false;
	case CNodeKind::STAT_EXPR:
		return UsedVariable(node->e.x, variable);
	case CNodeKind::EXPR_CALL:
		for (auto param : CListNode(*node->call.args))
		{
			if (UsedVariable(param, variable))
				return true;
		}
		break;
	case CNodeKind::STAT_WHILE:
	case CNodeKind::STAT_DO_WHILE:
		if (UsedVariable(node->s.condition, variable))
			return true;
		if (UsedVariable(node->s.then, variable))
			return true;
		return false;
	case CNodeKind::STAT_IF:
		if (UsedVariable(node->s.condition, variable))
			return true;
		if (UsedVariable(node->s.then, variable))
			return true;
		if (node->s._else)
			if (UsedVariable(node->s._else, variable))
				return true;
		return false;
	case CNodeKind::STAT_LABEL:
		return UsedVariable(node->l.body, variable);
	case CNodeKind::STAT_RETURN:
		if (node->e.x)
			return UsedVariable(node->e.x, variable);
		return false;
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
	case CNodeKind::EXPR_NOT:
		return UsedVariable(node->e.x, variable);
	case CNodeKind::EXPR_ADD:
	case CNodeKind::EXPR_SUB:
	case CNodeKind::EXPR_BOR:
	case CNodeKind::EXPR_BAND:
	case CNodeKind::EXPR_XOR:
	case CNodeKind::EXPR_SHIFT_LEFT:
	case CNodeKind::EXPR_SHIFT_RIGHT:
	case CNodeKind::EXPR_GREAT:
	case CNodeKind::EXPR_GREAT_EQUAL:
	case CNodeKind::EXPR_NOT_EQUAL:
	case CNodeKind::EXPR_EQUAL:
	case CNodeKind::EXPR_LESS:
	case CNodeKind::EXPR_LESS_EQUAL:
	case CNodeKind::EXPR_INDEX:
	case CNodeKind::EXPR_AND:
	case CNodeKind::EXPR_OR:
		if (UsedVariable(node->e.x, variable))
			return true;
		return UsedVariable(node->e.y, variable);

	case CNodeKind::EXPR_BOR_ASSIGN:
	case CNodeKind::EXPR_BAND_ASSIGN:
	case CNodeKind::EXPR_ASSIGN:
		return UsedVariable(node->e.y, variable);
	default:
	{
		Sprintf<> s;
		s.Format(_T("判断节点是否使用变量: 未实现的节点类型 %s"), ToString(node->kind));
		throw Exception(s.ToString());
		break;
	}
	}
}

// 获取变量的整数赋值语句，失败返回空
CNode* GetDefinedStatement(CNode* node, CNode* variable)
{
	if (node->kind != CNodeKind::STAT_EXPR)
		return nullptr;

	auto expr = node->e.x;
	// 要求是 变量 = 整数 这样的形式
	if (expr->kind != CNodeKind::EXPR_ASSIGN)
		return nullptr;

	auto left = expr->e.x;
	auto right = expr->e.y;
	if (!MatchVariableInteger(expr))
		return nullptr;

	if (left->variable->name == variable->variable->name)
		return node;
	return nullptr;
}

// 获取变量的赋值语句，失败返回空
CNode* GetVariableAssignment(CNode* node, CNode* variable)
{
	if (node->kind != CNodeKind::STAT_EXPR)
		return nullptr;

	auto expr = node->e.x;
	// 要求是 变量 = 整数 这样的形式
	if (expr->kind != CNodeKind::EXPR_ASSIGN || !expr->e.x->IsVariable())
		return nullptr;

	if (expr->e.x->variable->name == variable->variable->name)
		return node;
	return nullptr;
}


// 查找do while 语句 node 的初始化语句，var 是迭代变量
CNode* CASTDoWhileOptimier::GetInitializeStatement(CNode* node, CNode* var)
{
	CNode* init = nullptr;
	for (auto prev = node->GetPrev(); prev; prev = prev->GetPrev())
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
CNode* CASTDoWhileOptimier::GetIteratorStatement(CNode* node, CNode* var)
{
	CNode* iter = nullptr;
	auto body = node->s.then;
	if (body->kind != CNodeKind::STAT_LIST)
		return nullptr;

	for (auto tail = body->list.tail; tail; tail = tail->GetPrev())
	{
		// 判断是否定值语句
		if ((iter = GetVariableAssignment(tail, var)) != nullptr)
			break;
		// 如果使用了迭代变量，就认为失败
		if (UsedVariable(tail, var))
			return nullptr;
	}
	return iter;
}

#include "Dump.h"

void CASTDoWhileOptimier::PreVisit(CNode* node, int depth)
{
	if (node->kind != CNodeKind::STAT_DO_WHILE)
		return;

	// (1) 判断条件表达式
	if (!MatchCompare(node->s.condition))
		return;

	auto cond = node->s.condition;
	CNode* var, *value;
	if (IsVariable(cond->e.x->kind))
	{
		var = cond->e.x;
		value = cond->e.y;
	}
	else
	{
		value = cond->e.x;
		var = cond->e.y;
	}

	// (2) 查找初始化语句
	CNode* init = GetInitializeStatement(node, var);
	if (!init)
		return;

	// (3) 查找迭代语句
	auto body = node->s.then;
	CNode* iter = GetIteratorStatement(node, var);
	if (!iter)
		return;

	// (4) 保证for循环至少执行一次
	auto initValue = init->e.x->e.y;  // 赋值语句的表达式的y操作数
	if (!Evaluate(cond->kind, *initValue, *value))
		return;

	// 可以转换为 for 语句
	// (1) 移除初始化语句
	auto parent = GetTraverser()->GetAncestors().back().node;
	CListNode(parent).Remove(init);
	// (2) 移除迭代语句
	CListNode(body).Remove(iter);
	// 修改 do while 为 for
	node->For(init->e.x, node->s.condition, iter->e.x, body);
}

