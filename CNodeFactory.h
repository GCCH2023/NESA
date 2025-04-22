#pragma once
#include "CNode.h"

// 创建 CNode 的工厂类
class CNodeFactory
{
public:
	CNodeFactory(Allocator& allocator);
	// 创建 for 语句
	CNode* For(CNode* init, CNode* condition, CNode* iter, CNode* body);
	// 创建 goto 语句
	CNode* Goto(String* label);
	// 创建 if 语句
	CNode* If(CNode* condition, CNode* body, CNode* _else = nullptr);
	// 创建 while 语句
	CNode* While(CNode* condition, CNode* body);
	// 创建 do while 语句
	CNode* DoWhile(CNode* condition, CNode* body);
	// 创建 标签 语句
	CNode* Label(String* label, CNode* body);
	// 创建 表达式 语句
	CNode* ExprStat(CNode* expr);
	// 创建整数
	CNode* Integer(int value);
	// 创建函数调用
	CNode* Call(String* function, CNode* params = nullptr);
	// 创建类型转换表达式
	CNode* Cast(const Type* type, CNode* expr);
	// 创建字段
	CNode* Field(const ::Field* field);
	// 创建表达式
	CNode* Expr(CNodeKind kind, CNode* x = nullptr, CNode* y = nullptr, CNode* z = nullptr);
	// 创建变量表达式
	CNode* Var(const Variable* variable);
	// 创建 复合语句
	CNode* ListStat(CNode* head, CNode* tail);
	// 创建赋值表达式
	CNode* Assign(CNode* target, CNode* source);
	// 创建赋值语句
	CNode* AssignStat(CNode* target, CNode* source);
	// 创建 空 语句
	CNode* EmptyStat();
	// 创建 return 语句
	CNode* Return(CNode* value);

	// 创建单目赋值表达式 z = op x
	CNode* UnaryAssignExpr(CNodeKind op, CNode* z, CNode* x);
	// 创建双目赋值表达式 z = x op y
	CNode* BinaryAssignExpr(CNodeKind op, CNode* z, CNode* x, CNode* y);
	// 创建单目赋值表达式语句 z = op x;
	CNode* UnaryAssignExprStat(CNodeKind op, CNode* z, CNode* x);
	// 创建双目赋值表达式语句 z = x op y;
	CNode* BinaryAssignExprStat(CNodeKind op, CNode* z, CNode* x, CNode* y);

	// 复制节点
	CNode* Copy(const CNode& node);
private:
	Allocator& allocator;
};

