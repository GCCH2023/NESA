#include "stdafx.h"
#include "CNode.h"

//CNode::CNode():
//kind(CNodeKind::NONE),
//_for({0})
//{
//}
//
//CNode::CNode(CNodeKind kind_):
//kind(kind_),
//_for({0})
//{
//}

const TCHAR* ToString(CNodeKind kind)
{
	switch (kind)
	{
	case CNodeKind::NONE: return _T("");
	case CNodeKind::STAT_LIST: return _T("STAT_LIST");
	case CNodeKind::STAT_EXPR: return _T("STAT_EXPR");
	case CNodeKind::STAT_WHILE: return _T("STAT_WHILE");
	case CNodeKind::STAT_EMPTY: return _T("STAT_NONE");
	case CNodeKind::STAT_DO_WHILE: return _T("STAT_DO_WHILE");
	case CNodeKind::STAT_FOR: return _T("STAT_FOR");
	case CNodeKind::STAT_IF: return _T("STAT_IF");
	case CNodeKind::STAT_GOTO: return _T("STAT_GOTO");
	case CNodeKind::STAT_LABEL: return _T("STAT_LABEL");
	case CNodeKind::STAT_RETURN: return _T("STAT_RETURN");

	case CNodeKind::EXPR_VARIABLE: return _T("EXPR_VARIABLE");
	case CNodeKind::EXPR_FIELD: return _T("EXPR_FIELD");
	case CNodeKind::EXPR_INTEGER: return _T("EXPR_INTEGER");

	case CNodeKind::EXPR_ADD: return _T("+");
	case CNodeKind::EXPR_SUB: return _T("-");
	case CNodeKind::EXPR_ASSIGN: return _T("=");
	case CNodeKind::EXPR_BOR: return _T("|");
	case CNodeKind::EXPR_BAND: return _T("&");
	case CNodeKind::EXPR_XOR: return _T("^");
	case CNodeKind::EXPR_SHIFT_LEFT: return _T("<<");
	case CNodeKind::EXPR_SHIFT_RIGHT: return _T(">>");
	case CNodeKind::EXPR_GREAT: return _T(">");
	case CNodeKind::EXPR_GREAT_EQUAL: return _T(">=");
	case CNodeKind::EXPR_NOT_EQUAL: return _T("!=");
	case CNodeKind::EXPR_EQUAL: return _T("==");
	case CNodeKind::EXPR_LESS: return _T("<");
	case CNodeKind::EXPR_LESS_EQUAL: return _T("<=");
	case CNodeKind::EXPR_INDEX: return _T("[]");
	case CNodeKind::EXPR_ARROW: return _T("->");
	case CNodeKind::EXPR_DOT: return _T(".");
	case CNodeKind::EXPR_DEREF: return _T("*");
	case CNodeKind::EXPR_ADDR: return _T("&");
	case CNodeKind::EXPR_CAST: return _T("(T)");
	case CNodeKind::EXPR_CALL: return _T("+");
	case CNodeKind::EXPR_BOR_ASSIGN: return _T("|=");
	case CNodeKind::EXPR_BAND_ASSIGN: return _T("&=");
	case CNodeKind::EXPR_AND: return _T("&&");
	case CNodeKind::EXPR_OR: return _T("||");
	case CNodeKind::EXPR_NOT: return _T("!");
	case CNodeKind::EXPR_BNOT: return _T("~");
	case CNodeKind::EXPR_CONDITION: return _T("?:");
	default:
		throw Exception(_T("未实现的 CNodeKind ToString"));
	}
}

int GetOperatorPriority(CNodeKind op)
{
	switch (op)
	{
		// 赋值运算符
	case CNodeKind::EXPR_ASSIGN:       // =
	case CNodeKind::EXPR_BOR_ASSIGN:   // |=
	case CNodeKind::EXPR_BAND_ASSIGN:  // &=
		return 16;

		// 逻辑或
	case CNodeKind::EXPR_OR:           // ||
		return 12;

		// 逻辑与
	case CNodeKind::EXPR_AND:          // &&
		return 11;

		// 位或
	case CNodeKind::EXPR_BOR:          // |
		return 10;

		// 异或
	case CNodeKind::EXPR_XOR:          // ^
		return 9;

		// 位与
	case CNodeKind::EXPR_BAND:         // &
		return 8;

		// 比较运算符
	case CNodeKind::EXPR_EQUAL:        // ==
	case CNodeKind::EXPR_NOT_EQUAL:    // !=
		return 7;

		// 关系运算符
	case CNodeKind::EXPR_LESS:         // <
	case CNodeKind::EXPR_LESS_EQUAL:   // <=
	case CNodeKind::EXPR_GREAT:        // >
	case CNodeKind::EXPR_GREAT_EQUAL:  // >=
		return 6;

		// 移位运算符
	case CNodeKind::EXPR_SHIFT_LEFT:   // <<
	case CNodeKind::EXPR_SHIFT_RIGHT:  // >>
		return 5;

		// 加减法
	case CNodeKind::EXPR_ADD:          // +
	case CNodeKind::EXPR_SUB:          // -
		return 4;

		// 一元运算符
	case CNodeKind::EXPR_DEREF:        // *
	case CNodeKind::EXPR_ADDR:         // &
	case CNodeKind::EXPR_CAST:         // (T)
		return 3;

		// 结构体访问运算符
	case CNodeKind::EXPR_DOT:         // .
	case CNodeKind::EXPR_ARROW:       // ->
	case CNodeKind::EXPR_INDEX:       // []
	case CNodeKind::EXPR_CALL:         // ()
		return 2;

		// 逻辑非
	case CNodeKind::EXPR_NOT:          // !
	case CNodeKind::EXPR_BNOT:          // ~
		return 1;

	case CNodeKind::EXPR_INTEGER:
	case CNodeKind::EXPR_VARIABLE:
	case CNodeKind::EXPR_FIELD:
		return 0;

		// 默认情况，表示无效运算符
	default:
		assert(false && "无效的运算符");
		return -1;
	}
}

int Evaluate(CNodeKind op, const Expression& x, const Expression& y)
{
	if (!x.IsInteger() || !y.IsInteger())
	{
		Sprintf<> s;
		s.Format(_T("C节点求值: 只能对整数节点求值"));
		throw Exception(s);
	}

	int a = x.GetInteger();
	int b = y.GetInteger();
	switch (op)
	{
	case CNodeKind::EXPR_ADD: return a + b;
	case CNodeKind::EXPR_SUB:  return a - b;
	case CNodeKind::EXPR_BOR:  return a | b;
	case CNodeKind::EXPR_BAND: return a & b;
	//case CNodeKind::EXPR_BNOT:  return ~a;
	case CNodeKind::EXPR_XOR: return a ^ b;
	case CNodeKind::EXPR_SHIFT_RIGHT: return a >> b;
	case CNodeKind::EXPR_SHIFT_LEFT: return a << b;
	case CNodeKind::EXPR_EQUAL: return a == b;
	case CNodeKind::EXPR_NOT_EQUAL: return a != b;
	case CNodeKind::EXPR_LESS: return a < b;
	case CNodeKind::EXPR_LESS_EQUAL: return a <= b;
	case CNodeKind::EXPR_GREAT: return a > b;
	case CNodeKind::EXPR_GREAT_EQUAL: return a >= b;
	}
	Sprintf<> s;
	s.Format(_T("C节点求值: 无法进行求值的操作码 %s"), ToString(op));
	throw Exception(s);
}