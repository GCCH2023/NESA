#include "stdafx.h"
#include "CNodeFactory.h"
#include "CNode.h"

CNodeFactory::CNodeFactory(Allocator& allocator_):
	allocator(allocator_)
{
}

CNode* CNodeFactory::For(CNode* init, CNode* condition, CNode* iter, CNode* body)
{
	CNode* node = allocator.Alloc<CNode>();
	node->For(init, condition, iter, body);
	return node;
}

CNode* CNodeFactory::Goto(String* label)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Goto(label);
	return node;
}

CNode* CNodeFactory::If(CNode* condition, CNode* body, CNode* _else)
{
	CNode* node = allocator.Alloc<CNode>();
	node->If(condition, body);
	return node;
}

CNode* CNodeFactory::While(CNode* condition, CNode* body)
{
	CNode* node = allocator.Alloc<CNode>();
	node->While(condition, body);
	return node;
}

CNode* CNodeFactory::DoWhile(CNode* condition, CNode* body)
{
	CNode* node = allocator.Alloc<CNode>();
	node->DoWhile(condition, body);
	return node;
}


CNode* CNodeFactory::Label(String* label, CNode* body)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Label(label, body);
	return node;
}

CNode* CNodeFactory::ExprStat(CNode* expr)
{
	CNode* node = allocator.Alloc<CNode>();
	node->ExprStat(expr);
	return node;
}

CNode* CNodeFactory::Integer(int value)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Integer(value);
	return node;
}

CNode* CNodeFactory::Call(String* function, CNode* params)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Call(function, params);
	return node;
}

CNode* CNodeFactory::Cast(const Type* type, CNode* expr)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Cast(type, expr);
	return node;
}

CNode* CNodeFactory::Field(const ::Field* field)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Field(field);
	return node;
}

CNode* CNodeFactory::Expr(CNodeKind kind, CNode* x, CNode* y, CNode* z)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Expr(kind, x, y, z);
	return node;
}

CNode* CNodeFactory::Var(const Variable* variable)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Var(variable);
	return node;
}

CNode* CNodeFactory::ListStat(CNode* head, CNode* tail)
{
	CNode* node = allocator.Alloc<CNode>();
	node->ListStat(head, tail);
	return node;
}

CNode* CNodeFactory::Assign(CNode* target, CNode* source)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Assign(target, source);
	return node;
}

CNode* CNodeFactory::AssignStat(CNode* target, CNode* source)
{
	CNode* expr = allocator.Alloc<CNode>();
	expr->Assign(target, source);
	CNode* node = allocator.Alloc<CNode>();
	node->ExprStat(node);
	return node;
}

CNode* CNodeFactory::EmptyStat()
{
	CNode* node = allocator.Alloc<CNode>();
	node->EmptyStat();
	return node;
}

CNode* CNodeFactory::Return(CNode* value)
{
	CNode* node = allocator.Alloc<CNode>();
	node->Return(value);
	return node;
}

CNode* CNodeFactory::UnaryAssignExpr(CNodeKind op, CNode* z, CNode* x)
{
	assert(IsUnaryExpression(op));

	return Assign(z, Expr(op, x));
}

CNode* CNodeFactory::BinaryAssignExpr(CNodeKind op, CNode* z, CNode* x, CNode* y)
{
	assert(IsBinaryExpression(op));

	return Assign(z, Expr(op, x, y));
}

CNode* CNodeFactory::UnaryAssignExprStat(CNodeKind op, CNode* z, CNode* x)
{
	return ExprStat(UnaryAssignExpr(op, z, x));
}

CNode* CNodeFactory::BinaryAssignExprStat(CNodeKind op, CNode* z, CNode* x, CNode* y)
{
	return ExprStat(BinaryAssignExpr(op, z, x, y));
}


CNode* CNodeFactory::Copy(const CNode& node)
{
	return allocator.New<CNode>(node);
}
