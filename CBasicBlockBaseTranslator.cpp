#include "stdafx.h"
#include "CBasicBlockBaseTranslator.h"
#include "TACFunction.h"
#include "CTranslator.h"
#include "Type.h"
#include "CDataBase.h"

CNode* CBasicBlockBaseTranslator::Translate(const TACBasicBlock* block)
{
	this->block = block;
	return OnTranslate(block);
}

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
	case	TACOperator::CALL:
		// 如果有参数，则必是 若干个 ARG 后面跟着一个 CALL
		// 直接出现 CALL，说明没有参数
		return TranslateCall(tac, nullptr);
	case	TACOperator::ARG:
	{
		// 若干个 ARG 后面跟着一个 CALL
		// 遇到 ARG，则要连着后面的直到 CALL 的三地址码一起翻译
		CNode* argsNode = GetNodeFactory().ExprList();
		CListNode args(argsNode);
		auto& codes = GetBasicBlock()->GetCodes();
		while (codes[index]->op == TACOperator::ARG)
		{
			args.Add(GetExpression(codes[index]->x));
			++index;
		}
		if (codes[index]->op != TACOperator::CALL)
			throw Exception(_T("三地址码翻译为C语句：ARG 后面不是 CALL"));
		// 最后是 CALL 指令
		return TranslateCall(codes[index], argsNode);
	}

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

CNode* CBasicBlockBaseTranslator::TranslateCall(const TAC* call, CNode* params)
{
	String* name = nullptr;
	if (call->x.IsAddress())  // 直接给出函数地址
	{
		Sprintf<> s;
		s.Format(_T("sub_%04X"), call->x.GetValue());
		name = GetCDB().AddString(s.ToString());
	}
	else if (call->x.IsTemp())  // 函数指针临时变量
	{
		name = GetTranslator()->GetLocalVariableName(call->x.GetValue());
	}
	else if (call->x.IsGlobal())  // 函数指针全局变量
	{
		uint32_t addr = call->x.GetValue();
		auto global = GetCDB().GetGlobalVariable(addr);
		if (!global)
		{
			Sprintf<> s;
			s.Format(_T("获取全局函数指针 %X 失败"), addr);
			throw Exception(s.ToString());
		}
		name = global->name;
	}
	else
	{
		Sprintf<> s;
		s.Format(_T("三地址码翻译为C语句：%04X 解析函数名称失败"), call->address);
		throw Exception(s.ToString());
	}
	CNode* expr = GetNodeFactory().Call(name, params);
	// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
	if (call->z.IsTemp())
	{
		expr = GetNodeFactory().Assign(GetExpression(call->z), expr);
	}
	return GetNodeFactory().ExprStat(expr);
}
