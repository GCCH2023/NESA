#include "stdafx.h"
#include "CASTTraverser.h"
#include "CNode.h"

void CASTTraverser::Traverse(CNode* node, CASTVisitor& visitor)
{
    if (!node) return;
	
    // 前序遍历
    visitor.PreVisit(node);

    // 根据节点类型调用特定的访问方法
	CNode* child;
	switch (node->kind)
	{
	case CNodeKind::EXPR_VARIABLE:
	case CNodeKind::EXPR_INTEGER:
	case CNodeKind::STAT_NONE:
	case CNodeKind::STAT_GOTO:
		break;
	case CNodeKind::STAT_LIST:
		for (child = node->list.head; child; child = child->GetNext())
			Traverse(child, visitor);
		break;
	case CNodeKind::STAT_EXPR:
		Traverse(node->e.x, visitor);
		break;
	case CNodeKind::EXPR_CALL:
		for (CNode* param = node->call.params; param; param = param->GetNext())
			Traverse(param, visitor);
		break;
	case CNodeKind::STAT_WHILE:
		Traverse(node->s.condition, visitor);
		Traverse(node->s.then, visitor);
		break;
	case CNodeKind::STAT_DO_WHILE:
		Traverse(node->s.condition, visitor);
		Traverse(node->s.then, visitor);
		break;
	case CNodeKind::STAT_IF:
		Traverse(node->s.condition, visitor);
		Traverse(node->s.then, visitor);
		if (node->s._else)
			Traverse(node->s._else, visitor);
		break;
	case CNodeKind::STAT_LABEL:
		Traverse(node->l.body, visitor);
		break;
	case CNodeKind::STAT_RETURN:
		if (node->e.x)
			Traverse(node->e.x, visitor);
		break;
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
	case CNodeKind::EXPR_NOT:
		Traverse(node->e.x, visitor);
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
		Traverse(node->e.x, visitor);
		Traverse(node->e.y, visitor);
		break;
	case CNodeKind::EXPR_CAST:
		Traverse(node->cast.expr, visitor);
		break;
	default:
	{
		Sprintf<> s;
		s.Format(_T("遍历抽象语法树: 未实现的节点类型 %s"), ToString(node->kind));
		throw Exception(s.ToString());
		break;
	}
	}

    // 后序遍历
    visitor.PostVisit(node);
}

