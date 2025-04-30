#include "stdafx.h"
#include "CBasicBlockBaseTranslator.h"
#include "TACFunction.h"
#include "CTranslator.h"
#include "Type.h"
#include "CDataBase.h"

Statement* CBasicBlockBaseTranslator::Translate(const TACBasicBlock* block)
{
	this->block = block;
	auto stat = OnTranslate(block);
	return stat;
}

Statement* CBasicBlockBaseTranslator::OnTranslate(const TACBasicBlock* block)
{
	CNode* expr = nullptr;
	std::vector<Statement*> list;
	auto& codes = block->GetCodes();
	for (size_t i = 0; i < codes.size(); ++i)
	{
		auto tac = codes[i];
		auto expr = TranslateTAC(tac, i);
		if (!expr)
			continue;

		list.push_back(GetNodeFactory().ExprStat(expr));
	}
	if (GetReturnStatement())
	{
		list.push_back(GetReturnStatement());
	}
	Nes::Address firstAddr = codes.empty() ? block->GetStartAddress() : codes[0]->address;
	auto ret = GetTranslator()->NewStatementList(list);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
	GetTranslator()->AddAddressMapStatement(firstAddr, ret);  // 记录下这个基本块对应的地址及语句

	return ret;
}


Expression* CBasicBlockBaseTranslator::TranslateTAC(const TAC* tac, size_t& index)
{
	Expression* expr;
	switch (tac->op)
	{
	case	TACOperator::BOR:
		return BinaryAssignExpression(CNodeKind::EXPR_BOR, tac);
	case	TACOperator::BAND:
		return BinaryAssignExpression(CNodeKind::EXPR_BAND, tac);
	case	TACOperator::ADD:
		return BinaryAssignExpression(CNodeKind::EXPR_ADD, tac);
	case	TACOperator::SUB:
		return BinaryAssignExpression(CNodeKind::EXPR_SUB, tac);
	case	TACOperator::XOR:
		return BinaryAssignExpression(CNodeKind::EXPR_XOR, tac);
	case	TACOperator::SHL:
		return BinaryAssignExpression(CNodeKind::EXPR_SHIFT_LEFT, tac);
	case	TACOperator::SHR:
		return BinaryAssignExpression(CNodeKind::EXPR_SHIFT_RIGHT, tac);
	case TACOperator::BOOL_GREAT:
		return BinaryAssignExpression(CNodeKind::EXPR_GREAT, tac);
	case TACOperator::BOOL_GEQ:
		return BinaryAssignExpression(CNodeKind::EXPR_GREAT_EQUAL, tac);
	case TACOperator::BOOL_LESS:
		return BinaryAssignExpression(CNodeKind::EXPR_LESS, tac);
	case TACOperator::BOOL_LEQ:
		return BinaryAssignExpression(CNodeKind::EXPR_LESS_EQUAL, tac);
	case TACOperator::BOOL_EQ:
		return BinaryAssignExpression(CNodeKind::EXPR_EQUAL, tac);
	case TACOperator::BOOL_NEQ:
		return BinaryAssignExpression(CNodeKind::EXPR_NOT_EQUAL, tac);
	case	TACOperator::ASSIGN:
		return AssignExpression(tac->z, GetExpression(tac->x));
	case TACOperator::BOOL_BAND:
		expr = BinaryExpression(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
		expr = BinaryExpression(CNodeKind::EXPR_NOT_EQUAL, expr, GetExpression(TACOperand(0)));
		return AssignExpression(tac->z, expr);
	case TACOperator::BOOL_BIT:
		expr = BinaryExpression(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y));
		expr = BinaryExpression(CNodeKind::EXPR_BAND, expr, GetExpression(TACOperand(1)));
		return AssignExpression(tac->z, expr);
	case TACOperator::CAST:
	{
		auto var = GetVariable(tac->z);
		expr = CastExpression(var->type, GetExpression(tac->x));
		return AssignExpression(tac->z, expr);
	}
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
		return AssignExpression(tac->z, right);
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
		return UnaryAssignExpression(CNodeKind::EXPR_ADDR, tac);
	case TACOperator::DEREF:
		return UnaryAssignExpression(CNodeKind::EXPR_DEREF, tac);
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
	case TACOperator::IFGEQ:  // 跳转指令是基本块的最后一条指令
		ConditionalJump(CNodeKind::EXPR_GREAT_EQUAL, tac);
		break;
	case TACOperator::IFGREAT:
		ConditionalJump(CNodeKind::EXPR_GREAT, tac);
		break;
	case TACOperator::IFEQ:
		ConditionalJump(CNodeKind::EXPR_EQUAL, tac);
		break;
	case TACOperator::IFNEQ:
		ConditionalJump(CNodeKind::EXPR_NOT_EQUAL, tac);
		break;
	case TACOperator::IFLESS:
		ConditionalJump(CNodeKind::EXPR_LESS, tac);
		break;
	case TACOperator::IFLEQ:
		ConditionalJump(CNodeKind::EXPR_LESS_EQUAL, tac);
		break;
	case TACOperator::IFTRUE:
		ConditionalJump(CNodeKind::EXPR_NOT_EQUAL, GetExpression(tac->x), GetExpression(TACOperand(0)), tac->z.GetValue());
		break;
	case TACOperator::IFFALSE:
		ConditionalJump(CNodeKind::EXPR_EQUAL, GetExpression(tac->x), GetExpression(TACOperand(0)), tac->z.GetValue());
		break;
	case TACOperator::GOTO:
	{
		// 新：当作条件总是真的跳转语句来翻译
		expr = GetExpression(TACOperand(1));
		SetJumpCondition(expr);
		SetJumpTarget(tac->z.GetValue());
		return nullptr;

		// 旧： goto 在控制流图中对应一条边，可能被处理成循环结构，也可能就是对应goto语句，
		// 还不知道该怎么处理
		//if (tac->z.GetValue() == tac->address)
		//{
		// // 跳转到自己的语句翻译为 while (1);
		// //expr = allocator.New<CInteger>(1);
		// //current = allocator.New<CWhileStatement>(expr, noneStatement);
		// current = noneStatement;
		// condition = allocator.New<CNode>(1);
		// break;
		//}
		//auto label = GetLabelName(tac->z.GetValue());
		//current = allocator.New<CNode>(CNodeKind::STAT_GOTO, label);
		break;
	}
	case TACOperator::RETURN:
	{
		// 目前只能返回 AXY 对象，所以可以这么判断有没有返回值
		Expression* value = tac->x.IsZero() ? nullptr : GetExpression(tac->x);
		Return(value);
		break;
	}


	case TACOperator::ROR:
	{
		// C语言中没有ROR运算符，翻译为函数调用好了
		// void Ror(int*, int)
		auto arg = UnaryExpression(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
		return Call(_T("Ror"), { arg, GetExpression(tac->y) }, 0);
	}
	case TACOperator::ROL:
	{
		// C语言中没有ROL运算符，翻译为函数调用好了
		// void Rol(int*, int)
		auto arg = UnaryExpression(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
		return Call(_T("Rol"), { arg, GetExpression(tac->y) }, 0);
	}
	case TACOperator::PUSH:
	{
		// 还不知道怎么翻译push，先翻译为函数调用吧
		return Call(_T("Push"), { GetExpression(tac->x) }, 0);
	}
	case TACOperator::POP:
	{
		// 还不知道怎么翻译pop，先翻译为函数调用吧
		return Call(_T("Pop"), {}, tac->z);
	}
	case TACOperator::BOOL_FLAGV:
	{
		// 翻译为函数调用
		return Call(_T("IsOverflow"), { GetExpression(tac->x) , GetExpression(tac->y)}, tac->z);
	}

	case TACOperator::CLI:
	{
		// 翻译为函数调用
		return Call(_T("Cli"), {}, 0);
	}
	case TACOperator::SEI:
	{
		// 翻译为函数调用
		return Call(_T("Sei"), {}, 0);
	}
	case TACOperator::CLD:
	{
		// 翻译为函数调用
		return Call(_T("Cld"), {}, 0);
	}
	case TACOperator::SED:
	{
		// 翻译为函数调用
		return Call(_T("Sed"), {}, 0);
	}
	default:
	{
		Sprintf<> s;
		s.Format(_T("三地址码转C语句：未实现的三地址码 %s"), ToString(tac->op));
		throw Exception(s.ToString());
	}
	}
	return nullptr;
}

Expression* CBasicBlockBaseTranslator::UnaryExpression(CNodeKind kind, Expression* x)
{
	return GetNodeFactory().Unary(kind, x);
}

Expression* CBasicBlockBaseTranslator::BinaryExpression(CNodeKind kind, Expression* x, Expression* y)
{
	return GetNodeFactory().Binary(kind, x, y);
}

Expression* CBasicBlockBaseTranslator::AssignExpression(const TACOperand& z, Expression* x)
{
	return GetNodeFactory().Assign(GetExpression(z), x);
}

Expression* CBasicBlockBaseTranslator::UnaryAssignExpression(CNodeKind kind, const TAC* tac)
{
	auto expr = UnaryExpression(kind, GetExpression(tac->x));
	return AssignExpression(tac->z, expr);
}

Expression* CBasicBlockBaseTranslator::BinaryAssignExpression(CNodeKind kind, const TAC* tac)
{
	auto expr = BinaryExpression(kind, GetExpression(tac->x), GetExpression(tac->y));
	return AssignExpression(tac->z, expr);
}



const Variable* CBasicBlockBaseTranslator::GetVariable(const TACOperand& operand)
{
	Expression node;
	translator->GetExpression(node, operand);
	assert(node.GetKind() == CNodeKind::EXPR_VARIABLE);
	return node.GetVariable();
}

Expression* CBasicBlockBaseTranslator::Call(const TCHAR* func, std::initializer_list<Expression*> args, const TACOperand& result)
{
	auto name = GetCDB().AddString(func);
	return CallExpression(name, args, result);
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

Expression* CBasicBlockBaseTranslator::ArrayAssign(Expression* z, Expression* x)
{
	return GetNodeFactory().Assign(z, x);
}

Expression* CBasicBlockBaseTranslator::CallExpression(String* func, ConstArgList& args, const TACOperand& result)
{
	Expression* expr = GetNodeFactory().Call(func);
	auto& callArgs = expr->GetArguments();
	for (auto arg : args)
	{
		callArgs.push_back(arg);
	}
	// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
	if (result.IsTemp())
		expr = AssignExpression(result, expr);
	return expr;
}

Expression* CBasicBlockBaseTranslator::TranslateCall(const TAC* call, ConstArgList& args)
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
	return CallExpression(name, args, call->z);
}

Expression* CBasicBlockBaseTranslator::CastExpression(const Type* type, Expression* value)
{
	return GetNodeFactory().Cast(type, value);
}

void CBasicBlockBaseTranslator::ConditionalJump(CNodeKind op, Expression* x, Expression* y, uint32_t jump)
{
	// 条件跳转指令必定是基本块结束指令
	auto condition = BinaryExpression(op, x, y);
	SetJumpCondition(condition);
	SetJumpTarget(jump);
}

void CBasicBlockBaseTranslator::ConditionalJump(CNodeKind op, const TAC* tac)
{
	return ConditionalJump(op, GetExpression(tac->x), GetExpression(tac->y), tac->z.GetValue());
}

void CBasicBlockBaseTranslator::Return(Expression* value)
{
	SetReturnStatement(GetNodeFactory().Return(value));
}
