#include "stdafx.h"
#include "CTreeVisitor.h"
#include "CNode.h"

CTreeVisitor::CTreeVisitor()
{
}


CTreeVisitor::~CTreeVisitor()
{
}

void CTreeVisitor::Visit(CNode* node)
{
	if (!node)
		return;

	OnVisit(node);
}

void CTreeVisitor::OnVisit(CNode* node)
{

}

void CTreeVisitor::VisitChildren(CNode* node)
{
	CNode* child;
	switch (node->kind)
	{
	case CNodeKind::EXPR_VARIABLE:
	case CNodeKind::EXPR_INTEGER:
	case CNodeKind::STAT_NONE:
	case CNodeKind::STAT_GOTO:
		break;
	case CNodeKind::STAT_LIST:
		for (child = node->list.head; child; child = child->next)
			OnVisit(child);
		break;
	case CNodeKind::STAT_EXPR:
		OnVisit(node->e.x);
		break;
	case CNodeKind::STAT_CALL:
		for (CNode* param = node->f.params; param; param = param->next)
			OnVisit(param);
		break;
	case CNodeKind::STAT_WHILE:
		OnVisit(node->s.condition);
		OnVisit(node->s.then);
		break;
	case CNodeKind::STAT_DO_WHILE:
		OnVisit(node->s.condition);
		OnVisit(node->s.then);
		break;
	case CNodeKind::STAT_IF:
		OnVisit(node->s.condition);
		OnVisit(node->s.then);
		if (node->s._else)
			OnVisit(node->s._else);
		break;
	case CNodeKind::STAT_LABEL:
		OnVisit(node->l.body);
		break;
	case CNodeKind::STAT_RETURN:
		if (node->e.x)
			OnVisit(node->e.x);
		break;
	case CNodeKind::EXPR_REF:
	case CNodeKind::EXPR_ADDR:
	case CNodeKind::EXPR_NOT:
		OnVisit(node->e.x);
		break;
	case CNodeKind::EXPR_ASSIGN:
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
	case CNodeKind::EXPR_BOR_ASSIGN:
	case CNodeKind::EXPR_BAND_ASSIGN:
	case CNodeKind::EXPR_AND:
	case CNodeKind::EXPR_OR:
		OnVisit(node->e.x);
		OnVisit(node->e.y);
		break;
	default:
		break;
	}
}
