#include "stdafx.h"
#include "CBasicBlockBaseTranslator.h"
#include "TACFunction.h"
#include "CTranslator.h"
#include "Type.h"
#include "CDataBase.h"

#include "Dump.h"
Statement* CBasicBlockBaseTranslator::Translate(const TACBasicBlock* block)
{
	this->block = block;
	if (block->GetStartAddress() == 0x8000)
	{
		int a = 0;
	}
	auto stat = OnTranslate(block);

	Sprintf<> s;
	s.Format(_T("\n基本块 %04X : \n"), block->GetStartAddress());
	COUT << s.ToString();
	COUT << stat << std::endl;
	COUT << _T("跳转条件: ") << GetJumpCondition() << std::endl;
	COUT << s.Format(_T("跳转目标: %04X\n"), GetJumpTarget());

	return stat;
}

Statement* CBasicBlockBaseTranslator::TranslateTAC(const TAC* tac, size_t& index)
{
	Expression* expr;
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
		Expression* right;
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
		   // expr = allocator.New<Expression>(CNodeKind::EXPR_ADDR, x);
		   // // (2) 强制类型转换为 (char*)&x
		   // expr = allocator.New<Expression>(TypeManager::pValue, expr);
		   // // (3) 可选的偏移字节 (char*)&x + offset
		   // if (offset > 0)
		   // {
			  //  Expression* offsetNode = allocator.New<Expression>(offset);
			  //  expr = allocator.New<Expression>(CNodeKind::EXPR_ADD, expr, offsetNode);
		   // }
		   // // 解引用
		   // expr = allocator.New<Expression>(CNodeKind::EXPR_DEREF, expr);
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
		Expression* left;
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
		   // expr = allocator.New<Expression>(CNodeKind::EXPR_ADDR, x);
		   // // (2) 强制类型转换为 (char*)&x
		   // expr = allocator.New<Expression>(TypeManager::pValue, expr);
		   // // (3) 可选的偏移字节 (char*)&x + offset
		   // if (offset > 0)
		   // {
			  //  Expression* offsetNode = allocator.New<Expression>(offset);
			  //  expr = allocator.New<Expression>(CNodeKind::EXPR_ADD, expr, offsetNode);
		   // }
		   // // 解引用
		   // expr = allocator.New<Expression>(CNodeKind::EXPR_DEREF, expr);
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
		return TranslateCall(tac, {});
	case	TACOperator::ARG:
	{
		// 若干个 ARG 后面跟着一个 CALL
		// 遇到 ARG，则要连着后面的直到 CALL 的三地址码一起翻译
		std::vector<Expression*> args;
		auto& codes = GetBasicBlock()->GetCodes();
		while (codes[index]->op == TACOperator::ARG)
		{
			args.push_back(GetExpression(codes[index]->x));
			++index;
		}
		if (codes[index]->op != TACOperator::CALL)
			throw Exception(_T("三地址码翻译为C语句：ARG 后面不是 CALL"));
		// 最后是 CALL 指令
		return TranslateCall(codes[index], args);
	}
	case TACOperator::ROR:
	{
		// C语言中没有ROR运算符，翻译为函数调用好了
		// void Ror(int*, int)
		auto call = GetNodeFactory().Call(GetCDB().AddString(_T("Ror")));
		Expression* params = GetNodeFactory().Unary(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
		call->GetArguments().push_back(params);
		call->GetArguments().push_back(GetExpression(tac->y));
		return GetNodeFactory().ExprStat(call);
	}
	case TACOperator::ROL:
	{
		// C语言中没有ROL运算符，翻译为函数调用好了
		// void Rol(int*, int)
		auto call = GetNodeFactory().Call(GetCDB().AddString(_T("Rol")));
		Expression* params = GetNodeFactory().Unary(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
		call->GetArguments().push_back(params);
		call->GetArguments().push_back(GetExpression(tac->y));
		return GetNodeFactory().ExprStat(call);
	}
	case TACOperator::PUSH:
	{
		// 还不知道怎么翻译push，先翻译为函数调用吧
		Expression* params = GetExpression(tac->x);
		auto call = GetNodeFactory().Call(GetCDB().AddString(_T("Push")));
		call->GetArguments().push_back(params);
		return GetNodeFactory().ExprStat(call);
	}
	case TACOperator::POP:
	{
		// 还不知道怎么翻译pop，先翻译为函数调用吧
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Pop")));
		return  GetNodeFactory().AssignStat(GetExpression(tac->z), expr);
	}
	case TACOperator::BOOL_FLAGV:
	{
		// 翻译为函数调用
		auto call = GetNodeFactory().Call(GetCDB().AddString(_T("IsOverflow")));
		call->GetArguments().push_back(GetExpression(tac->x));
		call->GetArguments().push_back(GetExpression(tac->y));
		return GetNodeFactory().ExprStat(call);
		return  GetNodeFactory().AssignStat(GetExpression(tac->z), call);
	}

	case TACOperator::CLI:
	{
		// 翻译为函数调用
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Cli")));
		return  GetNodeFactory().ExprStat(expr);
	}
	case TACOperator::SEI:
	{
		// 翻译为函数调用
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Sei")));
		return  GetNodeFactory().ExprStat(expr);
	}
	case TACOperator::CLD:
	{
		// 翻译为函数调用
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Cld")));
		return  GetNodeFactory().ExprStat(expr);
	}
	case TACOperator::SED:
	{
		// 翻译为函数调用
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Sed")));
		return  GetNodeFactory().ExprStat(expr);
	}
	}
	Sprintf<> s;
	s.Format(_T("三地址码转C语句：未实现的三地址码 %s"), ToString(tac->op));
	throw Exception(s.ToString());
}

Expression* CBasicBlockBaseTranslator::BinaryExpression(CNodeKind kind, Expression* x, Expression* y)
{
	return GetNodeFactory().Binary(CNodeKind::EXPR_BAND, x, y);
}

Statement* CBasicBlockBaseTranslator::BinAssignStatement(CNodeKind kind, const TAC* tac)
{
	return GetNodeFactory().BinaryAssignExprStat(kind, GetExpression(tac->z), GetExpression(tac->x), GetExpression(tac->y));
}

Statement* CBasicBlockBaseTranslator::UnaryAssignStatement(CNodeKind kind, const TAC* tac)
{
	return GetNodeFactory().UnaryAssignExprStat(kind, GetExpression(tac->z), GetExpression(tac->x));
}

Statement* CBasicBlockBaseTranslator::AssignStatement(const TACOperand& z, Expression* x)
{
	return GetNodeFactory().AssignStat(GetExpression(z), x);
}


const Variable* CBasicBlockBaseTranslator::GetVariable(const TACOperand& operand)
{
	Expression node;
	translator->GetExpression(node, operand);
	assert(node.GetKind() == CNodeKind::EXPR_VARIABLE);
	return node.GetVariable();
}

inline CNodeFactory& CBasicBlockBaseTranslator::GetNodeFactory()
{
	return translator->GetNodeFactory();
}

Expression* CBasicBlockBaseTranslator::GetExpression(const TACOperand& operand)
{
	return GetTranslator()->GetExpression(operand);
}

Expression* CBasicBlockBaseTranslator::FieldExpression(Expression* obj, const Field* field)
{
	auto fieldNode = GetNodeFactory().Field(field);
	return GetNodeFactory().Binary(CNodeKind::EXPR_DOT, obj, fieldNode);
}

Expression* CBasicBlockBaseTranslator::IndexExpression(Expression* array, Expression* index)
{
	return GetNodeFactory().Binary(CNodeKind::EXPR_INDEX, array, index);
}

Statement* CBasicBlockBaseTranslator::ArrayAssign(Expression* z, Expression* x)
{
	return GetNodeFactory().AssignStat(z, x);
}

Statement* CBasicBlockBaseTranslator::TranslateCall(const TAC* call, const std::vector<Expression*>& args)
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
	Expression* expr = GetNodeFactory().Call(name);
	auto& callArgs = expr->GetArguments();
	for (auto arg : args)
	{
		callArgs.push_back(arg);
	}
	// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
	if (call->z.IsTemp())
	{
		expr = GetNodeFactory().Assign(GetExpression(call->z), expr);
	}
	return GetNodeFactory().ExprStat(expr);
}
