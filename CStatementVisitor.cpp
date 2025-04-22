#include "stdafx.h"
#include "CStatementVisitor.h"
#include "CNode.h"

void CStatementVisitor::Visit(CNode* root)
{
	if (!root)
		return;

	assert(root->IsStatement());
	OnVisit(root);
}

void CStatementVisitor::VisitChildren(CNode* node)
{
	if (!node)
		return;

	switch (node->kind)
	{
	case CNodeKind::STAT_EMPTY:
		break;
	case CNodeKind::STAT_LABEL:
		OnVisit(node->l.body);
		break;
	case CNodeKind::STAT_EXPR:
		break;
	case CNodeKind::STAT_LIST:
		for (auto n = node->list.head; n; n = n->GetNext())
			OnVisit(n);
		break;
	case CNodeKind::STAT_WHILE:
		OnVisit(node->s.then);
		break;
	case CNodeKind::STAT_DO_WHILE:
		OnVisit(node->s.then);
		break;
	case CNodeKind::STAT_FOR:
		OnVisit(node->_for.body);
		break;
	case CNodeKind::STAT_IF:
		OnVisit(node->s.then);
		break;
	case CNodeKind::STAT_GOTO:
		OnVisit(node->l.body);
		break;
	case CNodeKind::STAT_RETURN:
		break;
	}
}

void CStatementVisitor::OnVisit(CNode* node)
{
	VisitChildren(node);
}
