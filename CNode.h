#pragma once

enum class CNodeKind
{
	NONE,  // 未确定
	NORMAL,  // 普通语句

	// 表达式子类
	EXPR_VARIABLE,  // 变量
	EXPR_INTEGER,  // 整数常量

	// 语句子类
	STAT_PAIR,  // 语句对
	STAT_LIST,  // 语句列表
	STAT_EXPR,  // 表达式语句
	STAT_CALL,  // 函数调用
	STAT_WHILE,  // while 语句
	STAT_NONE,  // 空语句
	STAT_DO_WHILE,  // do while 语句
	STAT_IF, // if 语句
	STAT_GOTO,
	STAT_LABEL,
	STAT_RETURN,

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

	EXPR_INDEX,  // 索引 z = x[y]
	EXPR_REF,  // 解引用 *p
	EXPR_ADDR,  // 取地址 &a

	EXPR_BOR_ASSIGN,  // |= 
	EXPR_BAND_ASSIGN,  // &=

	EXPR_AND,  // &&
	EXPR_OR,  // ||
	EXPR_NOT,  // !
};

enum VariableKind
{
	VAR_KIND_GLOBAL,  // 全局变量
	VAR_KIND_LOCAL,  // 局部变量
	VAR_KIND_FUNCTION,  // 函数
};

using CStr = TCHAR*;

// C语言语法节点
struct CNode
{
	CNodeKind kind;  // 节点的类型
	uint32_t address;  // 对应的NES地址

	
	union
	{
		struct
		{
			CStr name;
			CNode* body;
		}l;  // 标签语句
		struct
		{
			CStr name;  // 变量名
			VariableKind varKind;
		}v;
		struct
		{
			int value;
		}i;  // 整数常量
		struct
		{
			CNode* x;
			CNode* y;
			CNode* z;
		}e;  // 表达式或语句, 如果是语句，则x是条件表达式, y 是 then 语句，z是else 语句
		struct
		{
			CNode* condition;
			CNode* then;
			CNode* _else;
		}s;
		struct
		{
			CStr name;  // 函数名称
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
	CNode(CStr name, VariableKind varKind = VAR_KIND_GLOBAL);
	// 创建函数调用或标签语句
	CNode(CStr name, CNode* params);
	// 创建整数
	CNode(int value);
	// 创建表达式或语句
	CNode(CNodeKind kind, CNode* x = nullptr, CNode* y = nullptr, CNode* z = nullptr);
	// 创建goto语句
	CNode(CNodeKind kind, CStr name);
};

class Function
{
public:

	inline void SetBody(CNode* body) { this->body = body; }
	inline CNode* GetBody() { return body; }

public:
	CNode* body;  // 函数体
	String name;
};

// 指定缩进输出节点
OStream& DumpCNode(OStream& os, const CNode* obj, int indent);
// 输出节点
OStream& operator<<(OStream& os, const CNode* obj);