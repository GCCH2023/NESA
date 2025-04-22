#include "stdafx.h"
#include "CNode.h"

CNode::CNode()
{
}

CNode::CNode(CNodeKind kind_):
kind(kind_)
{
}

void CNode::RemoveStatement(CNode* statement)
{
	assert(kind == CNodeKind::STAT_LIST && statement);

	auto prev = statement->GetPrev();
	if (prev)
		prev->next = statement->next;
	statement->prev = nullptr;
	auto next = statement->next;
	if (next)
		next->prev = prev;
	statement->next = nullptr;
	if (list.head == statement)
		list.head = next;
	if (list.tail == statement)
		list.tail = prev;
}

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

CNode& CNode::For(CNode* init, CNode* condition, CNode* iter, CNode* body)
{
	assert(init && init->IsExpression());
	assert(condition && condition->IsExpression());
	assert(iter && iter->IsExpression());
	assert(body && body->IsStatement());

	this->kind = CNodeKind::STAT_FOR;
	this->_for.init = init;
	this->_for.condition = condition;
	this->_for.iter = iter;
	this->_for.body = body;
	return *this;
}

CNode& CNode::Goto(String* label)
{
	assert(label);

	this->kind = CNodeKind::STAT_GOTO;
	this->l.name = label;
	return *this;
}

CNode& CNode::If(CNode* condition, CNode* body, CNode* _else)
{
	assert(condition && condition->IsExpression());
	assert(body && body->IsStatement());

	this->kind = CNodeKind::STAT_IF;
	this->s.condition = condition;
	this->s.then = body;
	this->s._else = _else;
	return *this;
}

CNode& CNode::While(CNode* condition, CNode* body)
{
	assert(condition && condition->IsExpression());
	assert(body && body->IsStatement());

	this->kind = CNodeKind::STAT_WHILE;
	this->s.condition = condition;
	this->s.then = body;
	return *this;
}

CNode& CNode::DoWhile(CNode* condition, CNode* body)
{
	assert(condition && condition->IsExpression());
	assert(body && body->IsStatement());

	this->kind = CNodeKind::STAT_DO_WHILE;
	this->s.condition = condition;
	this->s.then = body;
	return *this;
}

CNode& CNode::Return(CNode* value)
{
	if (value)
		assert(value->IsExpression());

	this->kind = CNodeKind::STAT_RETURN;
	this->e.x = value;
	return *this;
}

CNode& CNode::Label(String* label, CNode* body)
{
	assert(label);
	assert(body && body->IsStatement());

	this->kind = CNodeKind::STAT_LABEL;
	this->l.name = label;
	this->l.body = body;
	return *this;
}

CNode& CNode::ExprStat(CNode* expr)
{
	assert(expr && expr->IsExpression());

	this->kind = CNodeKind::STAT_EXPR;
	this->e.x = expr;
	return *this;
}

CNode& CNode::Integer(int value)
{
	this->kind = CNodeKind::EXPR_INTEGER;
	this->i.value = value;
	return *this;
}

CNode& CNode::Call(String* function, CNode* params)
{
	assert(function);
	assert(params && params->IsExpression());

	this->kind = CNodeKind::EXPR_CALL;
	this->call.name = function;
	this->call.params = params;
	return *this;
}

CNode& CNode::Cast(const Type* type, CNode* expr)
{
	assert(type);
	assert(expr && expr->IsExpression());

	this->kind = CNodeKind::EXPR_CAST;
	this->cast.type = type;
	this->cast.expr = expr;
	return *this;
}

CNode& CNode::Field(const ::Field* field)
{
	assert(field);

	this->kind = CNodeKind::EXPR_FIELD;
	this->field = field;
	return *this;
}

CNode& CNode::Expr(CNodeKind kind, CNode* x, CNode* y, CNode* z)
{
	assert(::IsExpression(kind));

	this->kind = kind;
	this->e.x = x;
	this->e.y = y;
	this->e.z = z;
	return *this;
}

CNode& CNode::Var(const Variable* variable)
{
	assert(variable);

	this->kind = CNodeKind::EXPR_VARIABLE;
	this->variable = variable;
	return *this;
}

CNode& CNode::ListStat(CNode* head, CNode* tail)
{
	assert(head && head->IsStatement());
	assert(tail && tail->IsStatement());

	this->kind = CNodeKind::STAT_LIST;
	this->list.head = head;
	this->list.tail = tail;
	return *this;
}

CNode& CNode::Assign(CNode* target, CNode* source)
{
	assert(target && target->IsExpression());
	assert(source && source->IsExpression());

	this->kind = CNodeKind::EXPR_ASSIGN;
	this->e.x = target;
	this->e.y = source;
	return *this;
}

CNode& CNode::EmptyStat()
{
	this->kind = CNodeKind::STAT_EMPTY;
	return *this;
}

CNode& CNode::Reset()
{
	this->kind = CNodeKind::NONE;
	return *this;
}
