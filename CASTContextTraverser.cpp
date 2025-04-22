#include "stdafx.h"
#include "CASTContextTraverser.h"
#include "CNode.h"

void CASTContextTraverser::Traverse(CNode* node, CASTContextVisitor& visitor)
{
	if (!node) return;

	visitor.SetTraverser(this);
	Reset();

	TraverseNode(node, visitor);
}

void CASTContextTraverser::TraverseNode(CNode* node, CASTContextVisitor& visitor)
{
	// 前序遍历
	visitor.PreVisit(node, depth);

	// 根据节点类型调用特定的访问方法
	CNode* child;
	switch (node->kind)
	{
	case CNodeKind::EXPR_VARIABLE:
	case CNodeKind::EXPR_INTEGER:
	case CNodeKind::STAT_EMPTY:
	case CNodeKind::STAT_GOTO:
		break;
	case CNodeKind::STAT_LIST:
		PushAncestor(node);
		for (child = node->list.head; child; child = child->GetNext())
		{
			TraverseNode(child, visitor);
			PushSenior(child);
		}
		PopAncestor();
		break;
	case CNodeKind::STAT_EXPR:
		PushAncestor(node);
		TraverseNode(node->e.x, visitor);
		PopAncestor();
		break;
	case CNodeKind::EXPR_CALL:
		PushAncestor(node);
		for (CNode* param = node->call.params; param; param = param->GetNext())
		{
			TraverseNode(param, visitor);
			PushSenior(param);
		}
		PopAncestor();
		break;
	case CNodeKind::STAT_WHILE:
	case CNodeKind::STAT_DO_WHILE:
		PushAncestor(node);
		TraverseNode(node->s.condition, visitor);
		TraverseNode(node->s.then, visitor);
		PopAncestor();
		break;
	case CNodeKind::STAT_FOR:
		PushAncestor(node);
		TraverseNode(node->_for.init, visitor);
		TraverseNode(node->_for.condition, visitor);
		TraverseNode(node->_for.iter, visitor);
		TraverseNode(node->_for.body, visitor);
		PopAncestor();
		break;
	case CNodeKind::STAT_IF:
		PushAncestor(node);
		TraverseNode(node->s.condition, visitor);
		TraverseNode(node->s.then, visitor);
		if (node->s._else)
			TraverseNode(node->s._else, visitor);
		PopAncestor();
		break;
	case CNodeKind::STAT_LABEL:
		PushAncestor(node);
		TraverseNode(node->l.body, visitor);
		PopAncestor();
		break;
	case CNodeKind::STAT_RETURN:
		if (node->e.x)
		{
			PushAncestor(node);
			TraverseNode(node->e.x, visitor);
			PopAncestor();
		}
		break;
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
	case CNodeKind::EXPR_NOT:
		PushAncestor(node);
		TraverseNode(node->e.x, visitor);
		PopAncestor();
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
		PushAncestor(node);
		TraverseNode(node->e.x, visitor);
		TraverseNode(node->e.y, visitor);
		PopAncestor();
		break;
	case CNodeKind::EXPR_CAST:
		PushAncestor(node);
		TraverseNode(node->cast.expr, visitor);
		PopAncestor();
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
	visitor.PostVisit(node, depth);
}

void CASTContextTraverser::Reset()
{
	ancestors.clear();
	seniors.clear();
}

void CASTContextTraverser::PopSeniors()
{
	while (!seniors.empty() && seniors.back().depth == depth)
		seniors.pop_back();
}
