#include "stdafx.h"
#include "CNode.h"

CNode::CNode(CNodeKind kind_, uint32_t address_) :
kind(kind_),
address(address_)
{

}

CNode::CNode(const Variable* variable_) :
kind(CNodeKind::EXPR_VARIABLE),
variable(variable_)
{
}


CNode::CNode(const Field* field_):
kind(CNodeKind::EXPR_FIELD),
field(field_)
{

}

CNode::CNode(const Function* func):
kind(CNodeKind::EXPR_FUNCTION),
function(func)
{

}




CNode::CNode(const Type* type_, CNode* expr_) :
kind(CNodeKind::EXPR_CAST)
{
	cast.type = type_;
	cast.expr = expr_;
}


CNode::CNode(int value) :
kind(CNodeKind::EXPR_INTEGER)
{
	i.value = value;
}

CNode::CNode(CNodeKind kind_, CNode* x, CNode* y /*= nullptr*/, CNode* z /*= nullptr*/) :
kind(kind_)
{
	e.x = x;
	e.y = y;
	e.z = z;
}

CNode::CNode(CNodeKind kind_, String* name) :
kind(kind_)
{
	l.name = name;
}

CNode::CNode()
{

}

const TCHAR* ToString(CNodeKind kind)
{
	switch (kind)
	{
	case CNodeKind::NONE: return _T("");
	case CNodeKind::STAT_LIST: return _T("STAT_LIST");
	case CNodeKind::STAT_EXPR: return _T("STAT_EXPR");
	case CNodeKind::STAT_WHILE: return _T("STAT_WHILE");
	case CNodeKind::STAT_NONE: return _T("STAT_NONE");
	case CNodeKind::STAT_DO_WHILE: return _T("STAT_DO_WHILE");
	case CNodeKind::STAT_IF: return _T("STAT_IF");
	case CNodeKind::STAT_GOTO: return _T("STAT_GOTO");
	case CNodeKind::STAT_LABEL: return _T("STAT_LABEL");
	case CNodeKind::STAT_RETURN: return _T("STAT_RETURN");

	case CNodeKind::EXPR_VARIABLE: return _T("EXPR_VARIABLE");
	case CNodeKind::EXPR_FIELD: return _T("EXPR_FIELD");
	case CNodeKind::EXPR_INTEGER: return _T("EXPR_INTEGER");
	case CNodeKind::EXPR_FUNCTION: return _T("EXPR_FUNCTION");


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
	default: return _T("");
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
		return 1;

	case CNodeKind::EXPR_INTEGER:
	case CNodeKind::EXPR_VARIABLE:
	case CNodeKind::EXPR_FIELD:
	case CNodeKind::EXPR_FUNCTION:
		return 0;

		// 默认情况，表示无效运算符
	default:
		assert(false && "无效的运算符");
		return -1;
	}
}