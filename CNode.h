#pragma once
#include "Variable.h"

struct String;
struct Field;

// 注意!!!
// 1. 增加节点类别时，注意修改相关的判断类型函数
// 2. 保证节点的大小一致，以便将一种节点修改为另一种节点
// 3. union 中的具体类型，应该将子节点放在前面，以便按数组方式访问

// 节点类别
enum class CNodeKind
{
	NONE,  // 未确定

	STAT_EMPTY,  // 空语句
	STAT_LABEL,
	STAT_EXPR,  // 表达式语句
	STAT_LIST,  // 复合语句

	STAT_WHILE,
	STAT_DO_WHILE,
	STAT_FOR,

	STAT_IF,

	STAT_GOTO,
	STAT_RETURN,

	EXPR_VARIABLE,  // 变量
	EXPR_FIELD,  // 字段
	EXPR_INTEGER,  // 整数常量

	EXPR_DEREF,  // 解引用 *x
	EXPR_ADDR,  // 取地址 &x
	EXPR_CAST,  // 类型转换 (T)a

	EXPR_NOT,  // !x

	EXPR_ADD,  // 加法 x + y
	EXPR_SUB,  // 减法 x - y

	EXPR_BOR,  // 位或 x | y
	EXPR_BAND,  // 位与 x & y
	EXPR_XOR,  // 异或 x ^ y
	EXPR_SHIFT_LEFT,  // 左移 x << y
	EXPR_SHIFT_RIGHT,  // 右移 x >> y

	EXPR_GREAT,  // 大于 x > y
	EXPR_GREAT_EQUAL,  // 大于等于 x >= y
	EXPR_NOT_EQUAL,  // 不等于 x != y
	EXPR_EQUAL,  // 等于 x == y
	EXPR_LESS,  // 小于 x < y
	EXPR_LESS_EQUAL,  // 小于等于 x <= y

	EXPR_ASSIGN, // 赋值 x = y
	EXPR_BOR_ASSIGN,  // x |= y
	EXPR_BAND_ASSIGN,  // x &= y

	EXPR_AND,  // x && y
	EXPR_OR,  // x || y

	EXPR_INDEX,  // 索引 x[y]
	EXPR_ARROW,  // 取记录对象指针的字段 x->y
	EXPR_DOT,  // 取记录对象的字段 x.y

	EXPR_CALL,  // 函数调用

	EXPR_CONDITION,  // 条件表达式 x ? y : z

	COUNT,  // 节点类别数量，不是有有效节点类别，必须是最后一个枚举值
};

// 获取节点类型的字符串表示
const TCHAR* ToString(CNodeKind kind);

// 节点类别分类
enum CNodeCategory : uint32_t
{
	CNODE_CAT_INVALID,  // 无效节点类别
	CNODE_CAT_EXPR = 0x10000000,  // 表达式
	CNODE_CAT_STAT = 0x20000000,  // 语句

	CNODE_CAT_EXPR_PRIMARY = CNODE_CAT_EXPR | 0x01000000,  // 初级表达式
	CNODE_CAT_EXPR_UNARY = CNODE_CAT_EXPR | 0x02000000,  // 单目表达式
	CNODE_CAT_EXPR_BINARY = CNODE_CAT_EXPR | 0x04000000,  // 双目表达式
	CNODE_CAT_EXPR_OTHER = CNODE_CAT_EXPR | 0x08000000,  // 其他表达式

	CNODE_CAT_EXPR_ARITH = CNODE_CAT_EXPR | 0x00010000,  // 算术表达式
	CNODE_CAT_EXPR_BITWISE = CNODE_CAT_EXPR | 0x00020000,  // 位运算表达式
	CNODE_CAT_EXPR_COMP = CNODE_CAT_EXPR | 0x00040000,  // 比较表达式
	CNODE_CAT_EXPR_LOGICAL = CNODE_CAT_EXPR | 0x00800000,  // 逻辑表达式
	CNODE_CAT_EXPR_OFFSET = CNODE_CAT_EXPR | 0x00100000,  // 偏移表达式

	CNODE_CAT_EXPR_ASSIGN = CNODE_CAT_EXPR | 0x00001000,  // 赋值表达式

	CNODE_CAT_STAT_LOOP = CNODE_CAT_STAT | 0x01000000,  // 循环语句
	CNODE_CAT_STAT_BRANCH = CNODE_CAT_STAT | 0x02000000,  // 分支语句
	CNODE_CAT_STAT_JUMP = CNODE_CAT_STAT | 0x04000000,  // 跳转语句
	CNODE_CAT_STAT_OTHER = CNODE_CAT_STAT | 0x08000000,  // 其他语句
};

// 获取节点分类
constexpr uint32_t GetCategory(CNodeKind kind)
{
	switch (kind)
	{
	case CNodeKind::STAT_EMPTY:
	case CNodeKind::STAT_LABEL:
	case CNodeKind::STAT_EXPR:
	case CNodeKind::STAT_LIST:
		return CNODE_CAT_STAT_OTHER;

	case CNodeKind::STAT_WHILE:
	case CNodeKind::STAT_DO_WHILE:
	case CNodeKind::STAT_FOR:
		return CNODE_CAT_STAT_LOOP;

	case CNodeKind::STAT_IF:
		return CNODE_CAT_STAT_BRANCH;

	case CNodeKind::STAT_GOTO:
	case CNodeKind::STAT_RETURN:
		return CNODE_CAT_STAT_JUMP;

	case CNodeKind::EXPR_VARIABLE:
	case CNodeKind::EXPR_FIELD:
	case CNodeKind::EXPR_INTEGER:
		return CNODE_CAT_EXPR_PRIMARY;

	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
	case CNodeKind::EXPR_CAST:
		return CNODE_CAT_EXPR_UNARY;

	case CNodeKind::EXPR_NOT:
		return CNODE_CAT_EXPR_UNARY | CNODE_CAT_EXPR_LOGICAL;

	case CNodeKind::EXPR_ADD:
	case CNodeKind::EXPR_SUB:
		return CNODE_CAT_EXPR_BINARY | CNODE_CAT_EXPR_ARITH;

	case CNodeKind::EXPR_BOR:
	case CNodeKind::EXPR_BAND:
	case CNodeKind::EXPR_XOR:
	case CNodeKind::EXPR_SHIFT_LEFT:
	case CNodeKind::EXPR_SHIFT_RIGHT:
		return CNODE_CAT_EXPR_BINARY | CNODE_CAT_EXPR_BITWISE;

	case CNodeKind::EXPR_GREAT:
	case CNodeKind::EXPR_GREAT_EQUAL:
	case CNodeKind::EXPR_NOT_EQUAL:
	case CNodeKind::EXPR_EQUAL:
	case CNodeKind::EXPR_LESS:
	case CNodeKind::EXPR_LESS_EQUAL:
		return CNODE_CAT_EXPR_BINARY | CNODE_CAT_EXPR_COMP;

	case CNodeKind::EXPR_ASSIGN:
		return CNODE_CAT_EXPR_BINARY | CNODE_CAT_EXPR_ASSIGN;

	case CNodeKind::EXPR_BOR_ASSIGN:
	case CNodeKind::EXPR_BAND_ASSIGN:
		return CNODE_CAT_EXPR_BINARY | CNODE_CAT_EXPR_ASSIGN | CNODE_CAT_EXPR_BITWISE;

	case CNodeKind::EXPR_AND:
	case CNodeKind::EXPR_OR:
		return CNODE_CAT_EXPR_BINARY | CNODE_CAT_EXPR_LOGICAL;

	case CNodeKind::EXPR_INDEX:
	case CNodeKind::EXPR_ARROW:
	case CNodeKind::EXPR_DOT:
		return CNODE_CAT_EXPR_BINARY | CNODE_CAT_EXPR_OFFSET;

	case CNodeKind::EXPR_CALL:
	case CNodeKind::EXPR_CONDITION:
		return CNODE_CAT_EXPR_OTHER;

	default:
		return CNODE_CAT_INVALID;
	}
}

// 辅助函数：检查分类是否匹配给定的掩码
constexpr bool MatchCategory(uint32_t category, uint32_t mask)
{
	return (category & mask) == mask;
}

// 是否是语句节点
constexpr bool IsStatement(CNodeKind kind)
{
	return MatchCategory(GetCategory(kind), CNODE_CAT_STAT);
}

// 是否是表达式节点
constexpr bool IsExpression(CNodeKind kind)
{
	return MatchCategory(GetCategory(kind), CNODE_CAT_EXPR);
}

// 是否是比较表达式
constexpr bool IsCompareExpression(CNodeKind kind)
{
	return MatchCategory(GetCategory(kind), CNODE_CAT_EXPR_COMP);
}

// 是否是变量
constexpr bool IsVariable(CNodeKind kind)
{
	return kind == CNodeKind::EXPR_VARIABLE;
}

// 是否赋值运算符
constexpr bool IsAssignment(CNodeKind kind)
{
	return MatchCategory(GetCategory(kind), CNODE_CAT_EXPR_ASSIGN);
}

// 是否单目运算符
constexpr bool IsUnaryExpression(CNodeKind kind)
{
	return MatchCategory(GetCategory(kind), CNODE_CAT_EXPR_UNARY);
}

// 是否双目运算符
constexpr bool IsBinaryExpression(CNodeKind kind)
{
	return MatchCategory(GetCategory(kind), CNODE_CAT_EXPR_BINARY);
}

// 获取子节点数量
constexpr size_t GetChildrenCount(CNodeKind kind)
{
	switch (kind)
	{
		// 语句节点
	case CNodeKind::STAT_EMPTY: return 0;  // 空语句无子节点
	case CNodeKind::STAT_LABEL: return 1;  // label name + body (但根据你的设计，LabelStatement只有body)
	case CNodeKind::STAT_EXPR: return 1;  // 表达式语句只有1个表达式子节点
	case CNodeKind::STAT_LIST: return 0;  // 复合语句的子节点通过List<Statement>管理，不在此计数
	case CNodeKind::STAT_WHILE: return 2;  // condition + body
	case CNodeKind::STAT_DO_WHILE: return 2;  // condition + body
	case CNodeKind::STAT_FOR: return 4;  // init + condition + iter + body
	case CNodeKind::STAT_IF: return 3;  // condition + then + else (else可能为空)
	case CNodeKind::STAT_GOTO: return 0;  // goto label无子节点
	case CNodeKind::STAT_RETURN: return 1;  // return value (可能为空)

		// 表达式节点
	case CNodeKind::EXPR_VARIABLE: return 0;  // 变量无子节点
	case CNodeKind::EXPR_FIELD: return 0;  // 字段无子节点
	case CNodeKind::EXPR_INTEGER: return 0;  // 整数常量无子节点

		// 单目运算符
	case CNodeKind::EXPR_DEREF: return 1;  // *x
	case CNodeKind::EXPR_ADDR: return 1;  // &x
	case CNodeKind::EXPR_CAST: return 1;  // (T)x
	case CNodeKind::EXPR_NOT: return 1;  // !x

		// 双目运算符
	case CNodeKind::EXPR_ADD: return 2;  // x + y
	case CNodeKind::EXPR_SUB: return 2;  // x - y
	case CNodeKind::EXPR_BOR: return 2;  // x | y
	case CNodeKind::EXPR_BAND: return 2;  // x & y
	case CNodeKind::EXPR_XOR: return 2;  // x ^ y
	case CNodeKind::EXPR_SHIFT_LEFT: return 2;  // x << y
	case CNodeKind::EXPR_SHIFT_RIGHT: return 2;  // x >> y
	case CNodeKind::EXPR_GREAT: return 2;  // x > y
	case CNodeKind::EXPR_GREAT_EQUAL: return 2;  // x >= y
	case CNodeKind::EXPR_NOT_EQUAL: return 2;  // x != y
	case CNodeKind::EXPR_EQUAL: return 2;  // x == y
	case CNodeKind::EXPR_LESS: return 2;  // x < y
	case CNodeKind::EXPR_LESS_EQUAL: return 2;  // x <= y
	case CNodeKind::EXPR_ASSIGN: return 2;  // x = y
	case CNodeKind::EXPR_BOR_ASSIGN: return 2;  // x |= y
	case CNodeKind::EXPR_BAND_ASSIGN: return 2; // x &= y
	case CNodeKind::EXPR_AND: return 2;  // x && y
	case CNodeKind::EXPR_OR: return 2;  // x || y
	case CNodeKind::EXPR_INDEX: return 2;  // x[y]
	case CNodeKind::EXPR_ARROW: return 2;  // x->y
	case CNodeKind::EXPR_DOT: return 2;  // x.y

		// 其他表达式
	case CNodeKind::EXPR_CALL: return 2;  // 头节点和尾节点
	case CNodeKind::EXPR_CONDITION: return 3; // x ? y : z

		// 特殊节点
	default: return 0;  // 未知节点
	}
}


class CNode
{
public:
	// 获取节点类别
	inline CNodeKind GetKind() const { return kind; }
	// 是否语句
	inline bool IsStatement() const { return MatchCategory(GetCategory(kind), CNODE_CAT_STAT); }
	// 是否表达式
	inline bool IsExpression() const { return MatchCategory(GetCategory(kind), CNODE_CAT_EXPR); }

	// 获取子节点数量
	inline size_t GetChildrenCount() const { return ::GetChildrenCount(GetKind()); }
protected:
	CNode(CNodeKind kind_) :kind(kind_) {}
	void SetKind(CNodeKind kind_) { kind = kind_; }
	inline CNode* Next() const { return next; }
	inline CNode* Prev() const { return prev; }
private:
	// 只允许CListNode修改链表指针
	template<typename U> friend class List;
	// 设置链表属性
	inline void SetPrev(CNode* node)
	{
		assert(node != this);
		prev = node;
	}
	inline void SetNext(CNode* node)
	{
		assert(node != this);
		next = node; 
	}
private:
	CNodeKind kind;
	// 构成双向链表
	CNode* prev = nullptr;
	CNode* next = nullptr;
};

// 专门用于操作列表节点的类
template<typename T>
class List {
public:
	// 迭代器类
	class iterator {
	private:
		T* current;
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using reference = T*;

		iterator(T* node = nullptr) : current(node) {}

		reference operator*() const { return current; }
		pointer operator->() const { return current; }

		iterator& operator++() {
			if (current) current = current->GetNext();
			return *this;
		}

		iterator operator++(int) {
			iterator temp = *this;
			++(*this);
			return temp;
		}

		iterator& operator--() {
			if (current) current = current->GetPrev();
			return *this;
		}

		iterator operator--(int) {
			iterator temp = *this;
			--(*this);
			return temp;
		}

		bool operator==(const iterator& other) const {
			return current == other.current;
		}

		bool operator!=(const iterator& other) const {
			return current != other.current;
		}
	};

	class const_iterator {
	private:
		const T* current;
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = const T;
		using difference_type = std::ptrdiff_t;
		using pointer = const T*;
		using reference = const T*;

		const_iterator(const T* node = nullptr) : current(node) {}

		reference operator*() const { return current; }
		pointer operator->() const { return current; }

		const_iterator& operator++() {
			if (current) current = current->GetNext();
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator temp = *this;
			++(*this);
			return temp;
		}

		const_iterator& operator--() {
			if (current) current = current->GetPrev();
			return *this;
		}

		const_iterator operator--(int) {
			const_iterator temp = *this;
			--(*this);
			return temp;
		}

		bool operator==(const const_iterator& other) const {
			return current == other.current;
		}

		bool operator!=(const const_iterator& other) const {
			return current != other.current;
		}
	};

	// 构造函数
	List() = default;
	List(const List<T>& other) = delete;

	// 设置拥有此容器的节点
	void SetOwner(T* owner)
	{
		assert(owner && !this->owner);
		this->owner = owner;
	}

	// 容量
	bool empty() const { return head == nullptr; }
	size_t size() const {
		size_t count = 0;
		for (T* p = head; p; p = p->GetNext()) {
			++count;
		}
		return count;
	}

	// 元素访问
	T* front() const { return head; }
	T* back() const { return tail; }

	// 修改器
	void push_back(T* node) {
		if (!node) return;

		node->SetPrev(tail);
		node->SetNext(nullptr);

		if (tail) {
			tail->SetNext(node);
		}
		else {
			head = node;
		}
		tail = node;
		Check();
	}
	void push_front(T* node) {
		if (!node) return;

		node->SetPrev(nullptr);
		node->SetNext(head);

		if (head) {
			head->SetPrev(node);
		}
		else {
			tail = node;
		}
		head = node;
		Check();
	}
	void insert(const iterator& pos, T* node) {
		if (!node) return;

		if (pos == begin()) {
			push_front(node);
			return;
		}

		if (pos == end()) {
			push_back(node);
			return;
		}

		auto next = *pos;
		auto prev = next->GetPrev();
		node->SetPrev(prev);
		node->SetNext(next);

		if (prev) prev->SetNext(node);
		else head = node;

		if (next) next->SetPrev(node);
		else tail = node;

		Check();
	}
	// 将另一个List的所有元素移动到此List中
	void insert(const iterator& pos, List<T>&& other)
	{
		if (other.empty()) {
			return;  // 如果other为空，直接返回
		}

		// 获取插入位置的前驱和后继节点
		T* insert_prev = (pos != end()) ? pos->GetPrev() : tail;
		T* insert_next = (pos != end()) ? (*pos) : nullptr;

		// 连接other的头部
		other.head->SetPrev(insert_prev);
		if (insert_prev) {
			insert_prev->SetNext(other.head);
		}
		else {
			head = other.head;  // 插入到头部
		}

		// 连接other的尾部
		other.tail->SetNext(insert_next);
		if (insert_next) {
			insert_next->SetPrev(other.tail);
		}
		else {
			tail = other.tail;  // 插入到尾部
		}

		// 清空other，确保资源所有权转移
		other.head = nullptr;
		other.tail = nullptr;

		Check();  // 检查链表完整性
	}
	iterator erase(const iterator& pos) {
		if (pos == end()) return end();

		auto current = *pos;
		iterator nextIter(current->GetNext());
		auto prev = current->GetPrev();
		auto next = current->GetNext();

		if (next)
			next->SetPrev(prev);
		else
			tail = prev;
		if (prev)
			prev->SetNext(next);
		else
			head = next;

		current->SetPrev(nullptr);
		current->SetNext(nullptr);

		return nextIter;
	}
	void clear() noexcept {
		while (head) {
			CNode::ResetParent(head);

			T* next = head->GetNext();
			head->SetPrev(head->SetNext(nullptr));  // 只重置指针
			head = next;

		}
		tail = nullptr;
		Check();
	}
	void swap(List& other) noexcept {
		std::swap(head, other.head);
		std::swap(tail, other.tail);
	}

	void reverse() noexcept {
		if (!head || head == tail) return;

		T* current = head;
		while (current) {
			std::swap(current->GetPrev(), current->GetNext());
			current = current->GetPrev();  // 注意这里已经是原来的next
		}
		std::swap(head, tail);
	}

	// 迭代器
	iterator begin() noexcept { return iterator(head); }
	iterator end() noexcept { return iterator(nullptr); }
	const_iterator begin() const noexcept { return const_iterator(head); }
	const_iterator end() const noexcept { return const_iterator(nullptr); }
	const_iterator cbegin() const noexcept { return const_iterator(head); }
	const_iterator cend() const noexcept { return const_iterator(nullptr); }

	// 反向迭代器
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
	reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
	const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
	const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
	const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

protected:
	inline void Check()
	{
		assert(!head || head->GetPrev() == nullptr);
		assert(!tail || tail->GetNext() == nullptr);
	}
private:
	T* owner = nullptr;  // 包含列表的节点
	T* head = nullptr;
	T* tail = nullptr;
};


struct CNodeArray
{
	CNode* data[5];
};

struct Variable;
class Expression;

class VariableExpression
{
public:
	const Variable* variable;
};

class IntegerExpression
{
public:
	int value;
};

class FieldExpression
{
public:
	const Field* field;
};

class CastExpression
{
public:
	Expression* expr;
	const Type* type;
};

class UnaryExpression
{
public:
	Expression* x;
};

class BinaryExpression
{
public:
	Expression* x;
	Expression* y;
};

class ConditionExpression
{
public:
	Expression* x;
	Expression* y;
	Expression* z;
};

using Arguments = List<Expression>;

class CallExpression
{
public:
	Arguments args;
	String* func;

	CallExpression(String* function) :func(function) {}
};

class Expression : public CNode
{
public:
	// 默认创建非法节点
	Expression() :
		CNode(CNodeKind::NONE),
		children{ 0 }
	{
	}
	Expression(const Expression& expr) :
		CNode(expr.GetKind()),
		children(expr.children)
	{
	}
	Expression(Expression&& other) noexcept :
		CNode(other.GetKind()),
		children(other.children)
	{
		other.children = { nullptr };
	}
	// 获取前驱节点，可能为空
	Expression* GetPrev() const { return static_cast<Expression*>(Prev()); }
	// 获取后继节点，可能为空
	Expression* GetNext() const { return static_cast<Expression*>(Next()); }

	Expression& operator=(const Expression&) = delete;
	Expression& operator=(Expression&& other)
	{
		SetKind(other.GetKind());
		children = other.children;
		other.children = { nullptr };
		return *this;
	}
	// 根据索引获取子节点
	Expression* GetChild(size_t index)
	{
		assert(index < GetChildrenCount());
		return static_cast<Expression*>(children.data[index]);
	}

	static Expression Variable(const ::Variable* variable)
	{
		assert(variable);

		Expression expr(CNodeKind::EXPR_VARIABLE);
		expr.var = VariableExpression{ variable };
		return expr;
	}
	static Expression Integer(int value)
	{
		Expression expr(CNodeKind::EXPR_INTEGER);
		expr.integer = IntegerExpression{ value };
		return expr;
	}

	static Expression Field(const ::Field* field)
	{
		assert(field);

		Expression expr(CNodeKind::EXPR_FIELD);
		expr.field = FieldExpression{ field };
		return expr;
	}

	static Expression Cast(const Type* type, Expression* expression)
	{
		assert(type);
		assert(expression);

		Expression expr(CNodeKind::EXPR_CAST);
		expr.cast = CastExpression{ expression, type };
		return expr;
	}

	static Expression Unary(CNodeKind op, Expression* x)
	{
		assert(::IsUnaryExpression(op));
		assert(x);

		Expression expr(op);
		expr.unary = UnaryExpression{ x };
		return expr;
	}

	static Expression Binary(CNodeKind op, Expression* x, Expression* y)
	{
		assert(::IsBinaryExpression(op));
		assert(x && y);

		Expression expr(op);
		expr.binary = BinaryExpression{ x, y };
		return expr;
	}

	static Expression Ternary(CNodeKind op, Expression* x, Expression* y, Expression* z)
	{
		assert(x && y && z);

		Expression expr(op);
		expr.cond = ConditionExpression{ x , y, z };
		return expr;
	}

	static Expression Call(String* func)
	{
		assert(func);

		Expression expr(CNodeKind::EXPR_CALL);
		expr.call = CallExpression{ func };
		expr.call.args.SetOwner(&expr);
		return expr;
	}

	// 获取整数表达式的整数
	inline int GetInteger() const
	{
		assert(IsInteger());
		return integer.value;
	}
	// 获取变量表达式的变量
	inline const ::Variable* GetVariable() const
	{
		assert(IsVariable());
		return var.variable;
	}
	// 获取字段表达式的字段
	inline const ::Field* GetField() const
	{
		assert(IsField());
		return field.field;
	}
	// 获取函数调用表达式的函数名
	inline String* GetFunctionName() const
	{
		assert(IsCall());
		return call.func;
	}
	// 获取函数调用表达式的参数列表
	inline Arguments& GetArguments()
	{
		assert(IsCall());
		return call.args;
	}
	inline const Arguments& GetArguments() const
	{
		assert(IsCall());
		return call.args;
	}
	// 获取单目表达式的操作数
	inline Expression* GetOperand() const
	{
		assert(IsUnary());
		return unary.x;
	}
	// 获取双目表达式的左操作数
	inline Expression* GetLeftOperand() const
	{
		assert(IsBinary());
		return binary.x;
	}
	// 获取双目表达式的右操作数
	inline Expression* GetRightOperand() const
	{
		assert(IsBinary());
		return binary.y;
	}
	// 获取条件表达式的条件
	inline Expression* GetCondition() const
	{
		assert(IsCondition());
		return cond.x;
	}
	// 获取条件表达式的真值
	inline Expression* GetTrueValue() const
	{
		assert(IsCondition());
		return cond.y;
	}
	// 获取条件表达式的假值
	inline Expression* GetFalseValue() const
	{
		assert(IsCondition());
		return cond.z;
	}
	// 获取类型转换表达式的类型
	inline const Type* GetCastType() const
	{
		assert(IsCast());
		return cast.type;
	}
	// 获取类型转换表达式的值
	inline Expression* GetCastValue() const
	{
		assert(IsCast());
		return cast.expr;
	}

	// 是否比较表达式
	bool IsCompare() const { return MatchCategory(GetCategory(GetKind()), CNODE_CAT_EXPR_COMP); }
	// 是否整数
	bool IsInteger() const { return GetKind() == CNodeKind::EXPR_INTEGER; }
	// 是否变量
	bool IsVariable() const { return GetKind() == CNodeKind::EXPR_VARIABLE; }
	// 是否字段
	bool IsField() const { return GetKind() == CNodeKind::EXPR_FIELD; }
	// 是否函数调用表达式
	bool IsCall() const { return GetKind() == CNodeKind::EXPR_CALL; }
	// 是否单目表达式
	inline bool IsUnary() const
	{
		return MatchCategory(GetCategory(GetKind()), CNODE_CAT_EXPR_UNARY);
	}
	// 是否双目表达式
	inline bool IsBinary() const
	{
		return MatchCategory(GetCategory(GetKind()), CNODE_CAT_EXPR_BINARY);
	}
	// 是否三目表达式
	inline bool IsCondition() const
	{
		return GetKind() == CNodeKind::EXPR_CONDITION;
	}
	// 是否类型转换表达式
	inline bool IsCast() const
	{
		return GetKind() == CNodeKind::EXPR_CAST;
	}
	// 是否赋值表达式
	inline bool IsAssign() const
	{
		return MatchCategory(GetCategory(GetKind()), CNODE_CAT_EXPR_ASSIGN);
	}
	// 是否初级表达式
	inline bool IsPrimary() const
	{
		return MatchCategory(GetCategory(GetKind()), CNODE_CAT_EXPR_PRIMARY);
	}
private:
	Expression(CNodeKind kind) : CNode(kind) {}
private:
	union
	{
		CNodeArray children;  // 占位
		VariableExpression var;
		IntegerExpression integer;
		FieldExpression field;
		CastExpression cast;
		UnaryExpression unary;
		BinaryExpression binary;
		ConditionExpression cond;
		CallExpression call;
	};
};


class Statement;

class IfStatement
{
public:
	Expression* condition;
	Statement* then;
	Statement* else_;
};

class WhileStatement
{
public:
	Expression* condition;
	Statement* body;
};

class DoWhileStatement
{
public:
	Expression* condition;
	Statement* body;
};

class LabelStatement
{
public:
	Statement* body;
	String* name;
};

class ForStatement
{
public:
	Expression* init;  // 初始化表达式
	Expression* condition;  // 循环条件表达式
	Expression* iter;  // 迭代表达式
	Statement* body;  // 循环体语句
};

class GotoStatement
{
public:
	String* label;
};

class ReturnStatement
{
public:
	Expression* value;
};

class ExprStatement
{
public:
	Expression* expr;
};

class CompoundStatement
{
public:
	List<Statement> statements;
};

class Statement : public CNode
{
public:
	// 默认创建非法节点
	Statement() :
		CNode(CNodeKind::NONE),
		children{ 0 }
	{
	}
	Statement(const Statement& stat) :
		CNode(stat.GetKind()),
		children(stat.children)
	{
	}
	Statement(Statement&& other) noexcept :
		CNode(other.GetKind()),
		children(other.children)
	{
		other.children = { nullptr };
	}
	Statement& operator=(const Statement&) = delete;
	// 语句赋值，不会修改链接关系
	Statement& operator=(Statement&& other) noexcept
	{
		SetKind(other.GetKind());
		children = other.children;
		other.children = { nullptr };
		return *this;
	}
	// 获取前驱节点，可能为空
	Statement* GetPrev() const { return static_cast<Statement*>(Prev()); }
	// 获取后继节点，可能为空
	Statement* GetNext() const { return static_cast<Statement*>(Next()); }
	// 根据索引获取子节点
	CNode* GetChild(size_t index)
	{
		assert(index < GetChildrenCount());
		return children.data[index];
	}

	static Statement If(Expression* condition, Statement* then, Statement* _else = nullptr)
	{
		assert(condition);
		assert(then);

		Statement statement(CNodeKind::STAT_IF);
		statement.if_ = IfStatement{ condition, then, _else };
		return statement;
	}

	static Statement While(Expression* condition, Statement* body)
	{
		assert(condition);
		assert(body);

		Statement statement(CNodeKind::STAT_WHILE);
		statement.while_ = WhileStatement{ condition, body };
		return statement;
	}

	static Statement DoWhile(Expression* condition, Statement* then)
	{
		assert(condition);
		assert(then);

		Statement statement(CNodeKind::STAT_DO_WHILE);
		statement.doWhile = DoWhileStatement{ condition, then };
		return statement;
	}

	static Statement Empty()
	{
		Statement statement(CNodeKind::STAT_EMPTY);
		return statement;
	}

	static Statement Label(String* name, Statement* body)
	{
		assert(name);
		assert(body);

		Statement statement(CNodeKind::STAT_LABEL);
		statement.label = LabelStatement{ body, name };
		return statement;
	}

	static Statement For(Statement* body, Expression* init = nullptr, Expression* condition = nullptr, Expression* iter = nullptr)
	{
		assert(body);

		Statement statement(CNodeKind::STAT_FOR);
		statement.for_ = ForStatement{ init, condition, iter, body };
		return statement;
	}

	static Statement Goto(String* label)
	{
		assert(label);

		Statement statement(CNodeKind::STAT_GOTO);
		statement.goto_ = GotoStatement{ label };
		return statement;
	}

	static Statement Return(Expression* value = nullptr)
	{
		Statement statement(CNodeKind::STAT_RETURN);
		statement.return_ = ReturnStatement{ value };
		return statement;
	}

	static Statement Expr(Expression* expr)
	{
		assert(expr);

		Statement statement(CNodeKind::STAT_EXPR);
		statement.expr = ExprStatement{ expr };
		return statement;
	}

	static Statement Compound()
	{
		Statement statement(CNodeKind::STAT_LIST);
		statement.list = CompoundStatement();
		return statement;
	}

	// 获取复合语句的列表
	inline List<Statement>& AsList()
	{
		assert(IsCompound());
		return list.statements;
	}
	inline const List<Statement>& AsList() const
	{
		assert(IsCompound());
		return list.statements;
	}
	// 获取表达式语句的表达式
	inline Expression* GetExpression() const
	{
		assert(IsExprStatement());
		return expr.expr;
	}
	// 获取循环语句的条件
	inline Expression* GetLoopCondition() const
	{
		switch (GetKind())
		{
		case CNodeKind::STAT_FOR: return for_.condition;
		case CNodeKind::STAT_WHILE: return while_.condition;
		case CNodeKind::STAT_DO_WHILE: return doWhile.condition;
		default: assert(IsLoop()); return nullptr;
		}
	}
	// 获取循环语句的循环体
	inline Statement* GetLoopBody() const
	{
		switch (GetKind())
		{
		case CNodeKind::STAT_FOR: return for_.body;
		case CNodeKind::STAT_WHILE: return while_.body;
		case CNodeKind::STAT_DO_WHILE: return doWhile.body;
		default: assert(IsLoop()); return nullptr;
		}
	}
	// 检查是否是 for 语句
	inline bool IsFor() const
	{
		return GetKind() == CNodeKind::STAT_FOR;
	}
	// 获取初始化表达式（可能为nullptr）
	inline Expression* GetForInit() const
	{
		assert(IsFor());
		return for_.init;
	}
	// 获取迭代表达式（可能为nullptr）
	inline Expression* GetForIter() const
	{
		assert(IsFor());
		return for_.iter;
	}
	// 检查是否是 if 语句
	inline bool IsIf() const
	{
		return GetKind() == CNodeKind::STAT_IF;
	}
	// 获取 if 语句的条件表达式
	inline Expression* GetIfCondition() const
	{
		assert(IsIf());
		return if_.condition;
	}
	// 获取 then 分支语句
	inline Statement* GetThen() const
	{
		assert(IsIf());
		return if_.then;
	}
	// 获取 else 分支语句（可能为nullptr）
	inline Statement* GetElse() const
	{
		assert(IsIf());
		return if_.else_;
	}
	// 检查是否有 else 分支
	inline bool HasElse() const
	{
		return IsIf() && (if_.else_ != nullptr);
	}
	// 是否是标签语句
	inline bool IsLabel() const
	{
		return GetKind() == CNodeKind::STAT_LABEL;
	}
	// 获取标签语句的名称
	inline String* GetLabelName() const
	{
		assert(IsLabel());
		return label.name;
	}
	// 获取标签语句的语句体
	inline Statement* GetLabelBody() const
	{
		assert(IsLabel());
		return label.body;
	}
	// 是否返回语句
	inline bool IsReturn() const
	{
		return GetKind() == CNodeKind::STAT_RETURN;
	}
	// 获取返回语句的返回值 (可能为nullptr)
	inline Expression* GetReturnValue() const
	{
		return return_.value;
	}
	// 获取 goto 语句的标签
	inline String* GetGotoLabelName() const
	{
		return goto_.label;
	}

	// 是否空语句
	inline bool IsEmpty() const { return GetKind() == CNodeKind::STAT_EMPTY; }
	// 是否复合语句
	inline bool IsCompound() const { return GetKind() == CNodeKind::STAT_LIST; }
	// 是否表达式语句
	inline bool IsExprStatement() const { return GetKind() == CNodeKind::STAT_EXPR; }
	// 是否while语句
	inline bool IsWhile() const { return GetKind() == CNodeKind::STAT_WHILE; }
	// 是否do while语句
	inline bool IsDoWhile() const { return GetKind() == CNodeKind::STAT_DO_WHILE; }
	// 是否循环语句
	inline bool IsLoop() const { return MatchCategory(GetCategory(GetKind()), CNODE_CAT_STAT_LOOP); }
	// 是否 goto 语句
	inline bool IsGoto() const { return GetKind() == CNodeKind::STAT_GOTO; }

private:
	Statement(CNodeKind kind) : CNode(kind) {}
private:
	union
	{
		CNodeArray children;
		ForStatement for_;
		IfStatement if_;
		WhileStatement while_;
		DoWhileStatement doWhile;
		LabelStatement label;
		GotoStatement goto_;
		ReturnStatement return_;
		ExprStatement expr;
		CompoundStatement list;
	};
};

static_assert(sizeof(Statement) == sizeof(Expression) && sizeof(Statement) == 64);

// 获取运算符的优先级
// 返回值越小，优先级越大
int GetOperatorPriority(CNodeKind op);

// 计算 x op y 的值
// 要求 x 和 y 都是整数节点
// 失败抛出异常
int Evaluate(CNodeKind op, const Expression& x, const Expression& y);