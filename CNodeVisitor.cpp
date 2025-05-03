#include "stdafx.h"
#include "CNodeVisitor.h"
#include "CNode.h"

void CNodeVisitor::Visit(CNode* root)
{
	if (!root) return;

	if (!BeforeVisit(root))
		return;

	if (root->IsExpression())
		OnVisit(static_cast<Expression*>(root));
	else
		OnVisit(static_cast<Statement*>(root));

	AfterVisit(root);
}

void CNodeVisitor::VisitChildren(CNode* node)
{
	if (!node) return;

	auto stat = static_cast<Statement*>(node);
	auto expr = static_cast<Expression*>(node);
	switch (node->GetKind())
	{
	case CNodeKind::NONE:
		break;
	case CNodeKind::STAT_EMPTY:
		break;
	case CNodeKind::STAT_LABEL:
		TryVisit(stat->GetLabelBody());
		break;
	case CNodeKind::STAT_EXPR:
		TryVisit(stat->GetExpression());
		break;
	case CNodeKind::STAT_LIST:
		for (auto n : stat->AsList())
			TryVisit(n);
		break;
	case CNodeKind::STAT_WHILE:
	case CNodeKind::STAT_DO_WHILE:
		TryVisit(stat->GetLoopCondition());
		TryVisit(stat->GetLoopBody());
		break;
	case CNodeKind::STAT_FOR:
		TryVisit(stat->GetForInit());
		TryVisit(stat->GetLoopCondition());
		TryVisit(stat->GetForIter());
		TryVisit(stat->GetLoopBody());
		break;
	case CNodeKind::STAT_IF:
		TryVisit(stat->GetIfCondition());
		TryVisit(stat->GetThen());
		TryVisit(stat->GetElse());
		break;
	case CNodeKind::STAT_GOTO:
		break;
	case CNodeKind::STAT_RETURN:
		TryVisit(stat->GetReturnValue());
		break;
	case CNodeKind::EXPR_VARIABLE:
		break;
	case CNodeKind::EXPR_FIELD:
		break;
	case CNodeKind::EXPR_INTEGER:
		break;
	case CNodeKind::EXPR_DEREF:
		break;
	case CNodeKind::EXPR_ADDR:
	case CNodeKind::EXPR_CAST:
	case CNodeKind::EXPR_NOT:
	case CNodeKind::EXPR_BNOT:
		TryVisit(expr->GetOperand());
		break;
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
	case CNodeKind::EXPR_ASSIGN:
	case CNodeKind::EXPR_BOR_ASSIGN:
	case CNodeKind::EXPR_BAND_ASSIGN:
	case CNodeKind::EXPR_AND:
	case CNodeKind::EXPR_OR:
	case CNodeKind::EXPR_INDEX:
	case CNodeKind::EXPR_ARROW:
	case CNodeKind::EXPR_DOT:
		TryVisit(expr->GetLeftOperand());
		TryVisit(expr->GetRightOperand());
		break;
	case CNodeKind::EXPR_CALL:
		for (auto arg : expr->GetArguments())
			TryVisit(arg);
		break;
	case CNodeKind::EXPR_CONDITION:
		TryVisit(expr->GetCondition());
		TryVisit(expr->GetTrueValue());
		TryVisit(expr->GetFalseValue());
		break;
	default:
	{
		Sprintf<> s;
		s.Format(_T("访问子节点: 未实现的节点类型 %s"), ToString(node->GetKind()));
		throw Exception(s.ToString());
	}
	}
}

bool CNodeVisitor::BeforeVisit(CNode* root)
{
	return true;
}

void CNodeVisitor::AfterVisit(CNode* root)
{
}

// 访问语句节点

void CNodeVisitor::OnVisit(Statement* node) { VisitChildren(node); }

// 访问表达式节点

void CNodeVisitor::OnVisit(Expression* node) { VisitChildren(node); }
