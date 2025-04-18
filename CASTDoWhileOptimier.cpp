#include "stdafx.h"
#include "CASTDoWhileOptimier.h"
#include "CNode.h"

void CASTDoWhileOptimier::Reset()
{
}

// 判断语句是否使用了指定变量
bool UsedVariable(CNode* node, CNode* variable)
{
	return false;
}

// 获取变量的赋值语句，失败返回空
CNode* GetDefinedStatement(CNode* node, CNode* variable)
{
	if (node->kind != CNodeKind::STAT_EXPR)
		return nullptr;
	auto expr = node->e.x;
	if (expr->IsAssignment() && expr->e.x->variable->name == variable->variable->name)
		return node;
	return nullptr;
}


void CASTDoWhileOptimier::PreVisit(CNode* node, int depth)
{
	if (node->kind != CNodeKind::STAT_DO_WHILE)
		return;

	if (!CheckCondition(node->s.condition))
		return;

	auto cond = node->s.condition;
	auto var = cond->e.x->IsVariable() ? cond->e.x : cond->e.y;
	CNode* init = nullptr;
	// 查找初始化语句
	for (auto it = node->GetPrev(); it; it = it->GetPrev())
	{
		if ((init = GetDefinedStatement(it, var)) != nullptr)
			break;
		return;
	}

	// 查找迭代语句
	auto body = node->s.then;
	CNode* iter = nullptr;
	for (auto it = body->list.tail; it; it = it->GetPrev())
	{
		if ((iter = GetDefinedStatement(it, var)) != nullptr)
			break;
		return;
	}

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
	return node->IsCompareExpression() && (GetCategory(node->e.x->kind) | GetCategory(node->e.y->kind)) == mask;
}
