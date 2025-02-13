#pragma once

enum class CNodeKind
{
	NONE,  // 未确定
	NORMAL,  // 普通语句

	// 表达式子类
	EXPR_VARIABLE,  // 变量
	EXPR_INTEGER,  // 整数常量

	// 语句子类
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

	EXPR_GREAT,  // 大于 >
	EXPR_GREAT_EQUAL,  // 大于等于 >=
	EXPR_NOT_EQUAL,  // 不等于 !=
	EXPR_EQUAL,  // 等于 ==
	EXPR_LESS,  // 小于 <
	EXPR_LESS_EQUAL,  // 小于等于 <=

	EXPR_INDEX,  // 索引 z = x[y]
	EXPR_REF,  // 解引用 *p

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

	CNode(CNodeKind kind, uint32_t address);
};

// C语言表达式节点
struct CExpression : public CNode
{
	CExpression(CNodeKind kind, uint32_t address = 0);
};

// 单目表达式
struct CSingleExpression : public CExpression
{
	CExpression* operand;

	CSingleExpression(CNodeKind kind, CExpression* operand);
};


// 双目表达式
struct CBinaryExpression : public CExpression
{
	CExpression* left;
	CExpression* right;

	CBinaryExpression(CNodeKind kind, CExpression* left, CExpression* right);
};

enum VariableKind
{
	VAR_KIND_GLOBAL,  // 全局变量
	VAR_KIND_LOCAL,  // 局部变量
	VAR_KIND_FUNCTION,  // 函数
};

// 变量
struct CVariable : public CExpression
{
	String name;
	VariableKind varKind;

	CVariable(LPCTSTR name, VariableKind kind = VAR_KIND_GLOBAL);
};

// 整数常量
struct CInteger : public CExpression
{
	int value;

	CInteger(int value);
};

// C语言语句节点
struct CStatement : public CNode
{
	CStatement(CNodeKind kind, uint32_t address = 0);
};

// 语句序列
struct CListStatement : public CStatement
{
	CStatement* first;
	CStatement* second;

	CListStatement(CStatement* first = nullptr, CStatement* second = nullptr);
};

// 表达式语句
struct CExpressionStatement : public CStatement
{
	CExpression* expression;

	CExpressionStatement(CExpression* expression = nullptr);
};

// 函数调用语句
struct CCallStatement : public CStatement
{
	CVariable* funcName;  // 函数名称
	CExpression** args;  // 函数参数数组，以 空结尾

	CCallStatement(CVariable* funcName = nullptr);
};

// 函数调用语句
struct CWhileStatement : public CStatement
{
	CExpression* expression;
	CStatement* body;

	CWhileStatement(CExpression* expression = nullptr, CStatement* body = nullptr);
};

// 函数调用语句
struct CDoWhileStatement : public CStatement
{
	CExpression* expression;
	CStatement* body;

	CDoWhileStatement(CExpression* expression = nullptr, CStatement* body = nullptr);
};

struct CIfStatement : public CStatement
{
	CExpression* condition;
	CStatement* body;
	CStatement* _else;

	CIfStatement(CExpression* condition = nullptr, CStatement* body = nullptr, CStatement* _else = nullptr);
};

struct CLabelStatement : public CStatement
{
	CStatement* statement;
	String name;

	CLabelStatement(LPCTSTR name, CStatement* statement = nullptr);
};


struct CGotoStatement : public CStatement
{
	String label;

	CGotoStatement(LPCTSTR label = nullptr);
};

struct CReturnStatement : public CStatement
{
	CExpression* value;  // 返回值，空代表没有返回值

	CReturnStatement(CExpression* value = nullptr);
};


class Function
{
public:

	inline void SetBody(CStatement* body) { this->body = body; }
	inline CStatement* GetBody() { return body; }

public:
	CStatement* body;  // 函数体
	String name;
};

// 指定缩进输出节点
OStream& DumpCNode(OStream& os, const CNode* obj, int indent);
// 输出节点
OStream& operator<<(OStream& os, const CNode* obj);