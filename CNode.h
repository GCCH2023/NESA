#pragma once
#include "Variable.h"

struct String;
struct Field;

// !!!增加节点类型时，注意修改CNode中的判断类型函数
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
	EXPR_LIST,  // 表达式列表

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
		return CNODE_CAT_EXPR_OTHER;
	case CNodeKind::EXPR_LIST:
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


class CNode
{
public:
	// 获取节点类别
	CNodeKind GetKind() const { return kind; }
	// 是否语句
	bool IsStatement() const { return MatchCategory(GetCategory(kind), CNODE_CAT_STAT); }
	// 是否表达式
	bool IsExpression() const { return MatchCategory(GetCategory(kind), CNODE_CAT_EXPR); }

protected:
	CNode(CNodeKind kind_) :kind(kind_) {}
private:
	CNodeKind kind;
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
	const Type* type;
	Expression* expr;
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

class TernaryExpression
{
public:
	Expression* x;
	Expression* y;
	Expression* z;
};

using Arguments = std::vector<Expression*>;

class CallExpression
{
public:
	String* func;
	Arguments args;
};


class Expression : public CNode
{
public:
	Expression(const Expression& expr) :
		CNode(expr.GetKind()),
		value(expr.value)
	{
	}
	Expression(Expression&& expr) :
		CNode(expr.GetKind()),
		value(expr.value)
	{
	}

	static Expression Variable(const ::Variable* variable)
	{
		assert(variable);

		Expression expr(CNodeKind::EXPR_VARIABLE);
		expr.value = VariableExpression{ variable };
		return expr;
	}
	static Expression Integer(int value)
	{
		Expression expr(CNodeKind::EXPR_INTEGER);
		expr.value = IntegerExpression{ value };
		return expr;
	}
	static Expression Field(const ::Field* field)
	{
		assert(field);

		Expression expr(CNodeKind::EXPR_FIELD);
		expr.value = FieldExpression{ field };
		return expr;
	}
	static Expression Cast(const Type* type, Expression* expression)
	{
		assert(type);
		assert(expression);

		Expression expr(CNodeKind::EXPR_CAST);
		expr.value = CastExpression{ type, expression };
		return expr;
	}
	static Expression Unary(CNodeKind op, Expression* x)
	{
		assert(::IsUnaryExpression(op));
		assert(x);

		Expression expr(op);
		expr.value = UnaryExpression{ x };
		return expr;
	}
	static Expression Binary(CNodeKind op, Expression* x, Expression* y)
	{
		assert(::IsBinaryExpression(op));
		assert(x);

		Expression expr(op);
		expr.value = UnaryExpression{ x };
		return expr;
	}
	static Expression Call(String* func)
	{
		assert(func);

		Expression expr(CNodeKind::EXPR_CALL);
		expr.value = CallExpression{ func };
		return expr;
	}

	template<typename T>
	const T& As() const { return std::get<T>(value); }

	// 是否比较表达式
	bool IsCompare() const { return MatchCategory(GetCategory(GetKind()), CNODE_CAT_EXPR_COMP); }
	// 是否整数
	bool IsInteger() const { return GetKind() == CNodeKind::EXPR_INTEGER; }
	// 是否变量
	bool IsVariable() const { return GetKind() == CNodeKind::EXPR_VARIABLE; }

private:
	Expression(CNodeKind kind) : CNode(kind) {}
private:
	std::variant<VariableExpression,
		IntegerExpression,
		FieldExpression,
		CastExpression,
		UnaryExpression,
		BinaryExpression,
		TernaryExpression,
		CallExpression> value;
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
	Statement* then;
};

class DoWhileStatement
{
public:
	Expression* condition;
	Statement* then;
};

class LabelStatement
{
public:
	String* name;
	Statement* body;
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

using Statements = std::vector<Statement*>;

class CompoundStatement
{
public:
	Statements statements;
};

class Statement : public CNode
{
public:
	Statement(const Statement& stat) :
		CNode(stat.GetKind()),
		value(stat.value)
	{
	}
	Statement(Statement&& stat) :
		CNode(stat.GetKind()),
		value(stat.value)
	{
	}

	static Statement If(Expression* condition, Statement* then, Statement* _else = nullptr)
	{
		assert(condition);
		assert(then);

		Statement statement(CNodeKind::STAT_IF);
		statement.value = IfStatement{ condition, then, _else };
		return statement;
	}
	static Statement While(Expression* condition, Statement* body)
	{
		assert(condition);
		assert(body);

		Statement statement(CNodeKind::STAT_WHILE);
		statement.value = WhileStatement{ condition, body };
		return statement;
	}
	static Statement DoWhile(Expression* condition, Statement* then)
	{
		assert(condition);
		assert(then);

		Statement statement(CNodeKind::STAT_DO_WHILE);
		statement.value = DoWhileStatement{ condition, then };
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
		statement.value = LabelStatement{ name, body };
		return statement;
	}
	static Statement For(Statement* body, Expression* init = nullptr, Expression* condition = nullptr, Expression* iter = nullptr)
	{
		assert(body);

		Statement statement(CNodeKind::STAT_FOR);
		statement.value = ForStatement{ init, condition, iter, body };
		return statement;
	}
	static Statement Goto(String* label)
	{
		assert(label);

		Statement statement(CNodeKind::STAT_GOTO);
		statement.value = GotoStatement{ label };
		return statement;
	}
	static Statement Return(Expression* value = nullptr)
	{
		Statement statement(CNodeKind::STAT_RETURN);
		statement.value = ReturnStatement{ value };
		return statement;
	}
	static Statement Expr(Expression* expr)
	{
		assert(expr);

		Statement statement(CNodeKind::STAT_EXPR);
		statement.value = ExprStatement{ expr };
		return statement;
	}
	static Statement Compound()
	{
		Statement statement(CNodeKind::STAT_LIST);
		statement.value = CompoundStatement();
		return statement;
	}

	template<typename T>
	const T& As() const { return std::get<T>(value); }

	// 是否空语句
	bool IsEmpty() const { return GetKind() == CNodeKind::STAT_EMPTY; }
	// 是否复合语句
	bool IsCompound() const { return GetKind() == CNodeKind::STAT_LIST; }

private:
	Statement(CNodeKind kind) : CNode(kind) {}
private:
	std::variant<std::monostate,
		IfStatement,
		WhileStatement,
		DoWhileStatement,
		LabelStatement,
		ForStatement,
		GotoStatement,
		ReturnStatement,
		ExprStatement,
		CompoundStatement> value;
};

// 获取运算符的优先级
// 返回值越小，优先级越大
int GetOperatorPriority(CNodeKind op);

// 计算 x op y 的值
// 要求 x 和 y 都是整数节点
// 失败抛出异常
int Evaluate(CNodeKind op, const Expression& x, const Expression& y);