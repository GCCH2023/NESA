#pragma once
#include "CNode.h"

// 创建 CNode 的工厂类
class CNodeFactory
{
public:
	CNodeFactory(Allocator& allocator);
	// 创建 for 语句
	Statement* For(Statement* body, Expression* init = nullptr, Expression* condition = nullptr, Expression* iter = nullptr);
	// 创建 goto 语句
	Statement* Goto(String* label);
	// 创建 if 语句
	Statement* If(Expression* condition, Statement* then, Statement* _else = nullptr);
	// 创建 while 语句
	Statement* While(Expression* condition, Statement* body);
	// 创建 do while 语句
	Statement* DoWhile(Expression* condition, Statement* body);
	// 创建 标签 语句
	Statement* Label(String* label, Statement* body);
	// 创建 表达式 语句
	Statement* ExprStat(Expression* expr);
	// 创建 复合语句
	Statement* CompoundStat();
	// 创建 空 语句
	Statement* EmptyStat();
	// 创建 return 语句
	Statement* Return(Expression* value = nullptr);

	// 创建整数
	Expression* Integer(int value);
	// 创建函数调用
	Expression* Call(String* function);
	// 创建类型转换表达式
	Expression* Cast(const Type* type, Expression* expr);
	// 创建字段
	Expression* Field(const ::Field* field);
	// 创建表达式
	Expression* Unary(CNodeKind op, Expression* x);
	Expression* Binary(CNodeKind op, Expression* x, Expression* y);
	// 创建变量表达式
	Expression* Var(const Variable* variable);
	// 创建赋值表达式
	Expression* Assign(Expression* target, Expression* source);
	// 创建赋值语句
	Statement* AssignStat(Expression* target, Expression* source);

	// 创建单目赋值表达式 z = op x
	inline Expression* UnaryAssignExpr(CNodeKind op, Expression* z, Expression* x)
	{
		auto expr = Unary(op, x);
		return Assign(z, expr);
	}
	// 创建双目赋值表达式 z = x op y
	inline Expression* BinaryAssignExpr(CNodeKind op, Expression* z, Expression* x, Expression* y)
	{
		return Assign(z, Binary(op, x, y));
	}
	// 创建单目赋值表达式语句 z = op x;
	inline Statement* UnaryAssignExprStat(CNodeKind op, Expression* z, Expression* x)
	{
		return ExprStat(UnaryAssignExpr(op, z, x));
;	}
	// 创建双目赋值表达式语句 z = x op y;
	inline Statement* BinaryAssignExprStat(CNodeKind op, Expression* z, Expression* x, Expression* y)
	{
		return ExprStat(BinaryAssignExpr(op, z, x, y));
	}

	// 复制语句节点，不会复制节点的链接关系
	inline Statement* Copy(const Statement* node)
	{
		return allocator.New<Statement>(*node);
	}
	// 复制表达式节点，不会复制节点的链接关系
	inline Expression* Copy(const Expression* node)
	{
		return allocator.New<Expression>(*node);
	}
private:
	Allocator& allocator;
};

