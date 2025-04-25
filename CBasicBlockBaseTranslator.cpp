#include "stdafx.h"
#include "CBasicBlockBaseTranslator.h"
#include "TAC.h"
#include "CTranslator.h"
#include "Type.h"

CNode* CBasicBlockBaseTranslator::TranslateTAC(const TAC* tac, size_t& index)
{
	CNode* expr;
	switch (tac->op)
	{
	case	TACOperator::BOR:
		return BinAssignStatement(CNodeKind::EXPR_BOR, tac);
	case	TACOperator::BAND:
		return BinAssignStatement(CNodeKind::EXPR_BAND, tac);
	case	TACOperator::ADD:
		return BinAssignStatement(CNodeKind::EXPR_ADD, tac);
	case	TACOperator::SUB:
		return BinAssignStatement(CNodeKind::EXPR_SUB, tac);
	case	TACOperator::XOR:
		return BinAssignStatement(CNodeKind::EXPR_XOR, tac);
	case	TACOperator::SHL:
		return BinAssignStatement(CNodeKind::EXPR_SHIFT_LEFT, tac);
	case	TACOperator::SHR:
		return BinAssignStatement(CNodeKind::EXPR_SHIFT_RIGHT, tac);
	case TACOperator::BOOL_GREAT:
		return BinAssignStatement(CNodeKind::EXPR_GREAT, tac);
	case TACOperator::BOOL_GEQ:
		return BinAssignStatement(CNodeKind::EXPR_GREAT_EQUAL, tac);
	case TACOperator::BOOL_LESS:
		return BinAssignStatement(CNodeKind::EXPR_LESS, tac);
	case TACOperator::BOOL_LEQ:
		return BinAssignStatement(CNodeKind::EXPR_LESS_EQUAL, tac);
	case TACOperator::BOOL_EQ:
		return BinAssignStatement(CNodeKind::EXPR_EQUAL, tac);
	case TACOperator::BOOL_NEQ:
		return BinAssignStatement(CNodeKind::EXPR_NOT_EQUAL, tac);
	case	TACOperator::ASSIGN:
		return AssignStatement(tac->z, GetExpression(tac->x));
	case TACOperator::ARRAY_GET:
	{
		// 可能是给结构体字段赋值
		auto variable = GetVariable(tac->x);
		auto type = variable->type;
		CNode* right;
		if (type->GetKind() == TypeKind::Struct)
		{
			// y 必定是整数
			assert(tac->y.IsInterger());
			// 根据偏移量查找字段
			auto field = type->GetField(tac->y.GetValue());
			right = FieldExpression(GetExpression(tac->x), field);
		}
		//else if (type->GetKind() == TypeKind::Pointer)
		//{
		   // // y 必定是整数，此时是用指针的高字节赋值 z = *((char*)&x + y)，因为指针占2个字节
		   // assert(tac->y.IsInterger());
		   // int offset = tac->y.GetValue();
		   // // (1) 取指针的地址 &x
		   // expr = allocator.New<CNode>(CNodeKind::EXPR_ADDR, x);
		   // // (2) 强制类型转换为 (char*)&x
		   // expr = allocator.New<CNode>(TypeManager::pValue, expr);
		   // // (3) 可选的偏移字节 (char*)&x + offset
		   // if (offset > 0)
		   // {
			  //  CNode* offsetNode = allocator.New<CNode>(offset);
			  //  expr = allocator.New<CNode>(CNodeKind::EXPR_ADD, expr, offsetNode);
		   // }
		   // // 解引用
		   // expr = allocator.New<CNode>(CNodeKind::EXPR_DEREF, expr);
		//}
		else
		{
			right = IndexExpression(GetExpression(tac->x), GetExpression(tac->y));
		}
		return AssignStatement(tac->z, right);
	}
	case TACOperator::ARRAY_SET:
	{
		// 可能是给结构体字段赋值
		auto variable = GetVariable(tac->x);
		auto type = variable->type;
		CNode* left;
		if (type->GetKind() == TypeKind::Struct)
		{
			// y 必定是整数
			assert(tac->y.IsInterger());
			// 根据偏移量查找字段
			auto field = type->GetField(tac->y.GetValue());
			left = FieldExpression(GetExpression(tac->x), field);
		}
		//else if (type->GetKind() == TypeKind::Pointer)
		//{
		   // COUT << tac;
		   // // y 必定是整数，此时是用指针的高字节赋值 z = *((char*)&x + y)，因为指针占2个字节
		   // assert(tac->y.IsInterger());
		   // int offset = tac->y.GetValue();
		   // // (1) 取指针的地址 &x
		   // expr = allocator.New<CNode>(CNodeKind::EXPR_ADDR, x);
		   // // (2) 强制类型转换为 (char*)&x
		   // expr = allocator.New<CNode>(TypeManager::pValue, expr);
		   // // (3) 可选的偏移字节 (char*)&x + offset
		   // if (offset > 0)
		   // {
			  //  CNode* offsetNode = allocator.New<CNode>(offset);
			  //  expr = allocator.New<CNode>(CNodeKind::EXPR_ADD, expr, offsetNode);
		   // }
		   // // 解引用
		   // expr = allocator.New<CNode>(CNodeKind::EXPR_DEREF, expr);
		//}
		else
		{
			left = IndexExpression(GetExpression(tac->x), GetExpression(tac->y));
		}
		return ArrayAssign(left, GetExpression(tac->z));
	}
	case TACOperator::ADDR:
		return UnaryAssignStatement(CNodeKind::EXPR_ADDR, tac);
	case TACOperator::DEREF:
		return UnaryAssignStatement(CNodeKind::EXPR_DEREF, tac);

	}
	Sprintf<> s;
	s.Format(_T("三地址码转C语句：未实现的三地址码 %s"), ToString(tac->op));
	throw Exception(s.ToString());
}

CNode* CBasicBlockBaseTranslator::BinaryExpression(CNodeKind kind, CNode* x, CNode* y)
{
	return GetNodeFactory().Expr(CNodeKind::EXPR_BAND, x, y);
}

CNode* CBasicBlockBaseTranslator::BinAssignStatement(CNodeKind kind, const TAC* tac)
{
	return GetNodeFactory().BinaryAssignExprStat(kind, GetExpression(tac->z), GetExpression(tac->x), GetExpression(tac->y));
}

CNode* CBasicBlockBaseTranslator::UnaryAssignStatement(CNodeKind kind, const TAC* tac)
{
	return GetNodeFactory().UnaryAssignExprStat(kind, GetExpression(tac->z), GetExpression(tac->x));
}

CNode* CBasicBlockBaseTranslator::AssignStatement(const TACOperand& z, CNode* x)
{
	return nullptr; return GetNodeFactory().AssignStat(GetExpression(z), x);
}


const Variable* CBasicBlockBaseTranslator::GetVariable(const TACOperand& operand)
{
	CNode node;
	translator->GetExpression(node, operand);
	assert(node.kind == CNodeKind::EXPR_VARIABLE);
	return node.variable;
}

inline CNodeFactory& CBasicBlockBaseTranslator::GetNodeFactory()
{
	return translator->GetNodeFactory(); 
}

CNode* CBasicBlockBaseTranslator::GetExpression(const TACOperand& operand)
{
	return GetTranslator()->GetExpression(operand);
}

CNode* CBasicBlockBaseTranslator::FieldExpression(CNode* obj, const Field* field)
{
	auto fieldNode = GetNodeFactory().Field(field);
	return GetNodeFactory().Expr(CNodeKind::EXPR_DOT, obj, fieldNode);
}

CNode* CBasicBlockBaseTranslator::IndexExpression(CNode* array, CNode* index)
{
	return GetNodeFactory().Expr(CNodeKind::EXPR_INDEX, array, index);
}

CNode* CBasicBlockBaseTranslator::ArrayAssign(CNode* z, CNode* x)
{
	return GetNodeFactory().AssignStat(z, x);
}
