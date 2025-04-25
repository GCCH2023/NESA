#include "stdafx.h"
#include "CNode.h"

CNode::CNode():
kind(CNodeKind::NONE),
_for({0})
{
}

CNode::CNode(CNodeKind kind_):
kind(kind_),
_for({0})
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
	case CNodeKind::EXPR_LIST: return _T("EXPR_LIST");
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
	assert(init == nullptr || init->IsExpression());
	assert(condition == nullptr || condition->IsExpression());
	assert(iter == nullptr || iter->IsExpression());
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
	if (_else == (CNode*)0x8)
	{
		int a = 0;
	}
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

CNode& CNode::Call(String* function, CNode* args)
{
	assert(function);
	if (args)
		assert(args->kind == CNodeKind::EXPR_LIST);

	this->kind = CNodeKind::EXPR_CALL;
	this->call.name = function;
	this->call.args = args;
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

CNode& CNode::ListStat()
{
	this->kind = CNodeKind::STAT_LIST;
	this->list.head = nullptr;
	this->list.tail = nullptr;
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

CNode& CNode::ExprList()
{
	this->kind = CNodeKind::EXPR_LIST;
	return *this;
}

CNode& CNode::Reset()
{
	this->kind = CNodeKind::NONE;
	return *this;
}

CNode& CNode::CopyData(const CNode& other)
{
	kind = other.kind;
	_for = other._for;
	return *this;
}

CListNode::CListNode(CNode& node) :
	list(&node)
{
	assert(node.kind == CNodeKind::STAT_LIST || node.kind == CNodeKind::EXPR_LIST);
}

CListNode::CListNode(CNode* node) :
	list(node)
{
	assert(node->kind == CNodeKind::STAT_LIST || node->kind == CNodeKind::EXPR_LIST);
}

CListNode& CListNode::Add(CNode* node)
{
	if (!node)
		return *this;

	//assert(!node->parent);

	if (list->list.head == nullptr)
	{
		list->list.head = list->list.tail = node;
	}
	else
	{
		list->list.tail->next = node;
		node->prev = list->list.tail;
		list->list.tail = node;
	}
	return *this;
}

CListNode& CListNode::PushFront(CNode* node)
{
	if (!node)
		return *this;

	if (list->list.head == nullptr)
	{
		list->list.head = list->list.tail = node;
	}
	else
	{
		node->next = list->list.head;
		list->list.head->prev = node;
		list->list.head = node;
	}
	return *this;
}

CListNode& CListNode::PushFront(CNode* head, CNode* tail)
{
	tail->next = list->list.head;
	list->list.head->prev = tail;
	list->list.head = head;

	return *this;
}

CListNode::Iterator CListNode::Remove(CNode* node)
{
	if (!node)
		return Iterator(nullptr);

	if (node->prev)
		node->prev->next = node->next;
	auto next = node->next;
	if (next)
		next->prev = node->prev;
	if (list->list.tail == node)
		list->list.tail = node->prev;
	if (list->list.head == node)
		list->list.head = next;
	node->prev = node->next = nullptr;
	return Iterator(next);
}

size_t CListNode::Count() const
{
	size_t count = 0;
	for (auto n = list->list.head; n; n = n->GetNext())
		++count;
	return count;
}

void CListNode::Add(const CListNode& other)
{
	auto head = other.list->list.head;
	auto tail = other.list->list.tail;

	if (list->list.head == nullptr)
	{
		list->list.head = head;
		list->list.tail = tail;
		return;
	}

	list->list.tail->next = head;
	head->prev = list->list.tail;
}

void CListNode::JoinFront(const CListNode& other)
{
	if (other.Count() == 0)
		return;
	
	PushFront(other.list->list.head, other.list->list.tail);
}

CListNode& CListNode::Insert(Iterator pos, const CListNode& other)
{
	if (other.Empty())
		return *this;

	if (pos == end())
	{
		Add(other);
		return *this;
	}

	auto p = *pos;
	auto head = other.list->list.head;
	auto tail = other.list->list.tail;

	tail->next = p->next;
	p->next->prev = tail;
	p->next = head;
	head->prev = p;
}


int Evaluate(CNodeKind op, const CNode& x, const CNode& y)
{
	if (!x.IsInteger() || !y.IsInteger())
	{
		Sprintf<> s;
		s.Format(_T("C节点求值: 只能对整数节点求值"));
		throw Exception(s);
	}

	int a = x.i.value;
	int b = y.i.value;
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
