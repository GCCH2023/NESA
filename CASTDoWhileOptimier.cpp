#include "stdafx.h"
#include "CASTDoWhileOptimier.h"
#include "CNode.h"

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
			for (CNode* child = node->list.head; child; child = child->GetNext())
				if ((child, variable))
					return true;
			return false;
		case CNodeKind::STAT_EXPR:
			return UsedVariable(node->e.x, variable);
		case CNodeKind::EXPR_CALL:
			for (CNode* param = node->call.params; param; param = param->GetNext())
				if (UsedVariable(param, variable))
					return true;
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
}

// 获取变量的赋值语句，失败返回空
CNode* GetDefinedStatement(CNode* node, CNode* variable)
{
	if (node->kind != CNodeKind::STAT_EXPR)
		return nullptr;
	auto expr = node->e.x;
	if (IsAssignment(expr->kind) && expr->e.x->variable->name == variable->variable->name)
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
	for (auto prev = node->GetPrev(); prev; prev = prev->GetPrev())
	{
		// 判断是否定值语句
		if ((iter = GetDefinedStatement(prev, var)) != nullptr)
			break;
		// 如果使用了迭代变量，就认为失败
		if (UsedVariable(prev, var))
			return nullptr;
	}
	return iter;
}



void CASTDoWhileOptimier::PreVisit(CNode* node, int depth)
{
	if (node->kind != CNodeKind::STAT_DO_WHILE)
		return;

	if (!CheckCondition(node->s.condition))
		return;

	auto cond = node->s.condition;
	auto var = IsVariable(cond->e.x->kind) ? cond->e.x : cond->e.y;

	CNode* init = GetInitializeStatement(node, var);
	if (!init)
		return;

	// 查找迭代语句
	auto body = node->s.then;
	CNode* iter = GetIteratorStatement(node, var);
	if (!iter)
		return;


	// 可以转换为 for 语句
	// (1) 移除初始化语句
	auto parent = GetTraverser()->GetAncestors().back().node;
	parent->RemoveStatement(init);
	// (2) 移除迭代语句
	body->RemoveStatement(iter);
	// 修改 do while 为 for
	CNode* condition = node->s.condition;
	node->kind = CNodeKind::STAT_FOR;
	node->_for.init = init->e.x;
	node->_for.condition = condition;
	node->_for.iter = iter->e.x;
	node->_for.body = body;
}

bool CASTDoWhileOptimier::CheckCondition(const CNode* node)
{
	// x op y，x，y中一个是变量，一个是整数
	// op 是关系运算符
	constexpr auto mask = GetCategory(CNodeKind::EXPR_INTEGER) | GetCategory(CNodeKind::EXPR_VARIABLE);
	return IsCompareExpression(node->kind) && (GetCategory(node->e.x->kind) | GetCategory(node->e.y->kind)) == mask;
}
