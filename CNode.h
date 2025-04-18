#pragma once
#include "Variable.h"

struct String;
struct Field;

// !!!增加节点类型时，注意修改CNode中的判断类型函数
// 以 _BEGIN 和 _END 结尾的枚举只用于划分区间，不要用作实际节点的类别
enum class CNodeKind
{
	NONE,  // 未确定

	/********/ STAT_BEGIN /********/,  // 语句开始

	STAT_LIST = STAT_BEGIN,  // 语句列表
	STAT_EXPR,  // 表达式语句
	STAT_WHILE,  // while 语句
	STAT_NONE,  // 空语句
	STAT_DO_WHILE,  // do while 语句
	STAT_IF, // if 语句
	STAT_GOTO,
	STAT_LABEL,
	STAT_RETURN,
	STAT_FOR,  // 

	/********/  STAT_END /********/,  // 语句结束


	/********/ EXPR_BEGIN /********/,  // 表达式开始

	/*11111*/ EXPR_PRIMARY_BEGIN = EXPR_BEGIN /*11111*/,  // 初级表达式开始
	EXPR_VARIABLE = EXPR_PRIMARY_BEGIN,  // 变量
	EXPR_FIELD,  // 字段
	EXPR_INTEGER,  // 整数常量
	/*11111*/ EXPR_PRIMARY_END /*11111*/,  // 初级表达式结束

	/*11111*/ EXPR_UNARY_BEGIN = EXPR_PRIMARY_END /*11111*/,  // 单目表达式开始
	EXPR_DEREF = EXPR_UNARY_BEGIN,  // 解引用 *x
	EXPR_ADDR,  // 取地址 &x
	EXPR_CAST,  // 类型转换 (T)a
	EXPR_NOT,  // !x
	/*11111*/ EXPR_UNARY_END /*11111*/,  // 单目表达式结束

	/*11111*/ EXPR_BINARY_BEGIN = EXPR_UNARY_END /*11111*/,  // 双目表达式开始

	/*22222*/ EXPR_BINARY_ARITH_BEGIN = EXPR_BINARY_BEGIN /*22222*/,  // 双目算术表达式开始
	EXPR_ADD = EXPR_BINARY_ARITH_BEGIN,  // 加法 x + y
	EXPR_SUB,  // 减法 x - y
	/*22222*/ EXPR_BINARY_ARITH_END /*22222*/,  // 双目算术表达式结束

	/*22222*/ EXPR_BINARY_BIT_BEGIN = EXPR_BINARY_ARITH_END /*22222*/,  // 双目位运算表达式开始
	EXPR_BOR = EXPR_BINARY_BIT_BEGIN,  // 位或 x | y
	EXPR_BAND,  // 位与 x & y
	EXPR_XOR,  // 异或 x ^ y
	EXPR_SHIFT_LEFT,  // 左移 x << y
	EXPR_SHIFT_RIGHT,  // 右移 x >> y
	/*22222*/ EXPR_BINARY_BIT_END /*22222*/,  // 双目位运算表达式结束

	/*22222*/ EXPR_BINARY_COMP_BEGIN = EXPR_BINARY_BIT_END /*22222*/,  // 双目比较表达式开始
	EXPR_GREAT = EXPR_BINARY_COMP_BEGIN,  // 大于 x > y
	EXPR_GREAT_EQUAL,  // 大于等于 x >= y
	EXPR_NOT_EQUAL,  // 不等于 x != y
	EXPR_EQUAL,  // 等于 x == y
	EXPR_LESS,  // 小于 x < y
	EXPR_LESS_EQUAL,  // 小于等于 x <= y
	/*22222*/ EXPR_BINARY_COMP_END /*22222*/,  // 双目比较表达式结束

	/*22222*/ EXPR_BINARY_ASSIGN_BEGIN = EXPR_BINARY_COMP_END /*22222*/,  // 双目赋值表达式开始
	EXPR_ASSIGN = EXPR_BINARY_ASSIGN_BEGIN, // 赋值 x = y
	EXPR_BOR_ASSIGN,  // x |= y
	EXPR_BAND_ASSIGN,  // x &= y
	/*22222*/ EXPR_BINARY_ASSIGN_END /*22222*/,  // 双目赋值表达式结束

	/*22222*/ EXPR_BINARY_LOGICAL_BEGIN = EXPR_BINARY_ASSIGN_END /*22222*/,  // 双目逻辑表达式开始
	EXPR_AND = EXPR_BINARY_LOGICAL_BEGIN,  // x && y
	EXPR_OR,  // x || y
	/*22222*/ EXPR_BINARY_LOGICAL_END /*22222*/,  // 双目逻辑表达式结束

	/*22222*/ EXPR_BINARY_VISIT_BEGIN = EXPR_BINARY_LOGICAL_END /*22222*/,  // 双目访问表达式开始
	EXPR_INDEX = EXPR_BINARY_VISIT_BEGIN,  // 索引 x[y]
	EXPR_ARROW,  // 取记录对象指针的字段 x->y
	EXPR_DOT,  // 取记录对象的字段 x.y
	/*22222*/ EXPR_BINARY_VISIT_END /*22222*/,  // 双目访问表达式结束

	/*11111*/ EXPR_BINARY_END /*11111*/,  // 双目表达式结束
	
	EXPR_CALL = EXPR_BINARY_END,  // 函数调用

	/********/ EXPR_END /********/,  // 表达式结束

	COUNT,  // 节点类别数量，不是有有效节点类别，必须是最后一个枚举值
};

// 获取节点类型的字符串表示
const TCHAR* ToString(CNodeKind kind);


static_assert(static_cast<int>(CNodeKind::COUNT) <= 64,	"CNodeKind values exceed uint64_t bit capacity");

// 获取节点类别分类
// 可将结果进行位运算后快速判断是否满足条件
constexpr uint64_t GetCategory(CNodeKind kind)
{
	// 验证有效性
	const auto val = static_cast<uint64_t>(kind);
	assert(val < static_cast<uint64_t>(CNodeKind::COUNT));

	// 确保安全位移
	return (val < 64) ? (1ull << val) : 0;
}

constexpr uint64_t StatementMask()
{
	uint64_t mask = 0;
	for (int i = static_cast<int>(CNodeKind::STAT_BEGIN); i < static_cast<int>(CNodeKind::STAT_END); ++i)
	{
		mask |= 1ull << i;
	}
	return mask;
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
	// 创建 for 语句
	CNode(CNode* init, CNode* condition, CNode* iter, CNode* body);
	
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

	// 是否是语句节点
	bool IsStatement() const { return kind >= CNodeKind::STAT_BEGIN && kind < CNodeKind::STAT_END; }
	// 是否是表达式节点
	bool IsExpression() const { return kind >= CNodeKind::EXPR_BEGIN && kind < CNodeKind::EXPR_END; }
	// 是否是比较表达式
	bool IsCompareExpression() const { return kind >= CNodeKind::EXPR_BINARY_COMP_BEGIN && kind < CNodeKind::EXPR_BINARY_COMP_END; }
	// 是否是变量
	bool IsVariable() const { return kind == CNodeKind::EXPR_VARIABLE; }
	// 是否赋值表达式
	bool IsAssignment() const { return kind >= CNodeKind::EXPR_BINARY_ASSIGN_BEGIN && kind < CNodeKind::EXPR_BINARY_ASSIGN_END; }
};

// 获取运算符的优先级
// 返回值越小，优先级越大
int GetOperatorPriority(CNodeKind op);
