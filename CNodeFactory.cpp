#include "stdafx.h"
#include "CNodeFactory.h"
#include "CNode.h"

CNodeFactory::CNodeFactory(Allocator& allocator_):
	allocator(allocator_)
{
}

Statement* CNodeFactory::For(Statement* body, Expression* init, Expression* condition, Expression* iter)
{
	return allocator.New<Statement>(Statement::For(body, init, condition, iter));
}

Statement* CNodeFactory::Goto(String* label)
{
	return allocator.New<Statement>(Statement::Goto(label));
}

Statement* CNodeFactory::If(Expression* condition, Statement* then, Statement* _else)
{
	return allocator.New<Statement>(Statement::If(condition, then, _else));
}

Statement* CNodeFactory::While(Expression* condition, Statement* body)
{
	return allocator.New<Statement>(Statement::While(condition, body));
}

Statement* CNodeFactory::DoWhile(Expression* condition, Statement* body)
{
	return allocator.New<Statement>(Statement::DoWhile(condition, body));
}


Statement* CNodeFactory::Label(String* label, Statement* body)
{
	return allocator.New<Statement>(Statement::Label(label, body));
}

Statement* CNodeFactory::ExprStat(Expression* expr)
{
	return allocator.New<Statement>(Statement::Expr(expr));
}

Statement* CNodeFactory::CompoundStat()
{
	return allocator.New<Statement>(Statement::Compound());
}

Statement* CNodeFactory::EmptyStat()
{
	return allocator.New<Statement>(Statement::Empty());
}

Statement* CNodeFactory::Return(Expression* value)
{
	return allocator.New<Statement>(Statement::Return(value));
}

Expression* CNodeFactory::Integer(int value)
{
	return allocator.New<Expression>(Expression::Integer(value));
}

Expression* CNodeFactory::Call(String* function)
{
	return allocator.New<Expression>(Expression::Call(function));
}

Expression* CNodeFactory::Cast(const Type* type, Expression* expr)
{
	return allocator.New<Expression>(Expression::Cast(type, expr));
}

Expression* CNodeFactory::Field(const ::Field* field)
{
	return allocator.New<Expression>(Expression::Field(field));
}

Expression* CNodeFactory::Unary(CNodeKind op, Expression* x)
{
	return allocator.New<Expression>(Expression::Unary(op, x));
}

Expression* CNodeFactory::Binary(CNodeKind op, Expression* x, Expression* y)
{
	return allocator.New<Expression>(Expression::Binary(op, x, y));
}

Expression* CNodeFactory::Var(const Variable* variable)
{
	return allocator.New<Expression>(Expression::Variable(variable));
}

Expression* CNodeFactory::Assign(Expression* target, Expression* source)
{
	return allocator.New<Expression>(Expression::Binary(CNodeKind::EXPR_ASSIGN, target, source));
}

Statement* CNodeFactory::AssignStat(Expression* target, Expression* source)
{
	Expression* expr = Assign(target, source);
	return ExprStat(expr);
}