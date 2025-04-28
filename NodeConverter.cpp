#include "stdafx.h"
#include "NodeConverter.h"

Expression* NodeConverter::Not(Expression* expr)
{
	CNodeKind kind = CNodeKind::NONE;
	switch (expr->GetKind())
	{
	case CNodeKind::EXPR_GREAT:
		kind = CNodeKind::EXPR_LESS_EQUAL;
		break;
	case CNodeKind::EXPR_GREAT_EQUAL:
		kind = CNodeKind::EXPR_LESS;
		break;
	case CNodeKind::EXPR_LESS:
		kind = CNodeKind::EXPR_GREAT_EQUAL;
		break;
	case CNodeKind::EXPR_LESS_EQUAL:
		kind = CNodeKind::EXPR_GREAT;
		break;
	case CNodeKind::EXPR_EQUAL:
		kind = CNodeKind::EXPR_NOT_EQUAL;
		break;
	case CNodeKind::EXPR_NOT_EQUAL:
		kind = CNodeKind::EXPR_EQUAL;
		break;
	default:
		throw Exception(_T("未实现的表达式取反类型"));
	}
	To(expr, Expression::Binary(kind, expr->GetLeftOperand(), expr->GetRightOperand()));
	return expr;
}
