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

// C语言语法节点
struct CNode
{
	CNodeKind kind;  // 节点的类型

	union
	{
		struct
		{
			String* name;
			CNode* body;
		}l;  // 标签语句
		const Variable* variable;  // 变量
		const Field* field;  // 记录类型的字段
		struct
		{
			const Type* type;  // 类型转换表达式的目标类型
			CNode* expr;
		}cast;  // 类型转换表达式
		struct
		{
			int value;
		}i;  // 整数常量
		struct
		{
			CNode* x;
			CNode* y;
			CNode* z;
		}e;  // 表达式的三个操作数
		struct
		{
			CNode* condition;
			CNode* then;
			CNode* _else;
		}s;  // if, while, do while
		struct
		{
			String* name;  // 函数名称
			CNode* params;  // 参数链表
		}call;
		struct
		{
			CNode* head;
			CNode* tail;
		}list; // 语句列表
		struct
		{
			CNode* condition;  // 循环条件表达式
			CNode* body;  // 循环体语句
			CNode* init;  // 初始化表达式
			CNode* iter;  // 迭代表达式
		}_for;  // for
	};

private:
	// 构成双向链表
	CNode* prev = nullptr;
	CNode* next = nullptr;
public:

	CNode();
	CNode(CNodeKind kind);
	CNode(const CNode& other) = default;

	// 设置为 for 语句
	CNode& For(CNode* init, CNode* condition, CNode* iter, CNode* body);
	// 设置为 goto 语句
	CNode& Goto(String* label);
	// 设置为 if 语句
	CNode& If(CNode* condition, CNode* body, CNode* _else = nullptr);
	// 设置为 while 语句
	CNode& While(CNode* condition, CNode* body);
	// 设置为 do while 语句
	CNode& DoWhile(CNode* condition, CNode* body);
	// 设置为 return 语句
	CNode& Return(CNode* value = nullptr);
	// 设置为 标签 语句
	CNode& Label(String* label, CNode* body);
	// 设置为 表达式 语句
	CNode& ExprStat(CNode* expr);
	// 设置为整数
	CNode& Integer(int value);
	// 设置为函数调用
	CNode& Call(String* function, CNode* params = nullptr);
	// 设置为类型转换表达式
	CNode& Cast(const Type* type, CNode* expr);
	// 设置为字段
	CNode& Field(const ::Field* field);
	// 设置为表达式
	CNode& Expr(CNodeKind kind, CNode* x = nullptr, CNode* y = nullptr, CNode* z = nullptr);
	// 设置为变量表达式
	CNode& Var(const Variable* variable);
	// 设置为 复合语句
	CNode& ListStat(CNode* head, CNode* tail);
	// 设置为赋值表达式
	CNode& Assign(CNode* target, CNode* source);
	// 设置为 空 语句
	CNode& EmptyStat();
	// 设置为无效节点
	CNode& Reset();

	// 是否语句
	bool IsStatement() const { return MatchCategory(GetCategory(kind), CNODE_CAT_STAT); }
	// 是否表达式
	bool IsExpression() const { return MatchCategory(GetCategory(kind), CNODE_CAT_EXPR); }


	// 将指定节点设置为后继节点，并将它的前驱设置为此节点
	void SetNext(CNode* node)
	{
		next = node;
		if (node)
			node->prev = this;
	/*	if (!node)
			return;
		node->next = next;
		node->prev = this;
		if (next)
			next->prev = node;
		next = node;*/
	}
	inline CNode* GetPrev() { return prev; }
	inline CNode* GetNext() { return next; }
	// 移除语句列表中的指定语句
	void RemoveStatement(CNode* statement);
};

// 获取运算符的优先级
// 返回值越小，优先级越大
int GetOperatorPriority(CNodeKind op);
