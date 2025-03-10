#pragma once
#include "Variable.h"

struct String;
struct Field;

// !!!增加节点类型时，注意修改CNode中的判断类型函数
enum class CNodeKind
{
	NONE,  // 未确定

	// 语句子类
	STAT_LIST,  // 语句列表
	STAT_EXPR,  // 表达式语句
	STAT_WHILE,  // while 语句
	STAT_NONE,  // 空语句
	STAT_DO_WHILE,  // do while 语句
	STAT_IF, // if 语句
	STAT_GOTO,
	STAT_LABEL,
	STAT_RETURN,

	// 表达式子类
	EXPR_VARIABLE,  // 变量
	EXPR_FIELD,  // 字段
	EXPR_INTEGER,  // 整数常量

	EXPR_ADD,  // 加法
	EXPR_SUB,  // 减法
	EXPR_ASSIGN, // 赋值
	EXPR_BOR,  // 位或 |
	EXPR_BAND,  // 位与 &
	EXPR_XOR,  // 异或 ^
	EXPR_SHIFT_LEFT,  // 左移 <<
	EXPR_SHIFT_RIGHT,  // 右移 >>

	EXPR_GREAT,  // 大于 >
	EXPR_GREAT_EQUAL,  // 大于等于 >=
	EXPR_NOT_EQUAL,  // 不等于 !=
	EXPR_EQUAL,  // 等于 ==
	EXPR_LESS,  // 小于 <
	EXPR_LESS_EQUAL,  // 小于等于 <=

	EXPR_INDEX,  // 索引 x[y]
	EXPR_ARROW,  // 取记录对象指针的字段 x->y
	EXPR_DOT,  // 取记录对象的字段 x.y
	EXPR_DEREF,  // 解引用 *p
	EXPR_ADDR,  // 取地址 &a
	EXPR_CAST,  // 类型转换 (T)a
	EXPR_CALL,  // 函数调用

	EXPR_BOR_ASSIGN,  // |= 
	EXPR_BAND_ASSIGN,  // &=

	EXPR_AND,  // &&
	EXPR_OR,  // ||
	EXPR_NOT,  // !
};


// C语言语法节点
struct CNode
{
	CNodeKind kind;  // 节点的类型
	uint32_t address;  // 对应的NES地址

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
		}f;
		struct
		{
			CNode* head;
			CNode* tail;
		}list; // 语句列表
	};
	CNode* next = nullptr;

	CNode();
	CNode(CNodeKind kind, uint32_t address);
	// 创建变量
	CNode(const Variable* variable);
	// 创建字段
	CNode(const Field* field);
	// 创建类型转换表达式
	CNode(const Type* type, CNode* expr);
	// 创建函数调用或标签语句
	CNode(String* name, CNode* params);
	// 创建整数
	CNode(int value);
	// 创建表达式或语句
	CNode(CNodeKind kind, CNode* x = nullptr, CNode* y = nullptr, CNode* z = nullptr);
	// 创建goto语句
	CNode(CNodeKind kind, String* name);

	// 是否是语句节点
	bool IsStatement() const { return kind >= CNodeKind::STAT_LIST && kind <= CNodeKind::STAT_RETURN; }
	// 是否是表达式节点
	bool IsExpression() const { return kind >= CNodeKind::EXPR_VARIABLE && kind <= CNodeKind::EXPR_NOT; }
};
