#include "stdafx.h"
#include "CBasicBlockTranslator.h"
#include "TACFunction.h"
#include "CNode.h"
#include "CDataBase.h"
#include "CTranslator.h"

CBasicBlockTranslator::CBasicBlockTranslator(CTranslator* translator_) :
	CBasicBlockBaseTranslator(translator_),
	condition(nullptr),
	jumpAddr(0)
{
}


CNode* CBasicBlockTranslator::TranslateCall(const TAC* call, CNode* params)
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

CNode* CBasicBlockTranslator::ConditionalJump(CNodeKind kind, const TAC* tac, uint32_t& jumpAddr)
{
	// 条件跳转指令必定是基本块结束指令
	condition = GetNodeFactory().Expr(kind, GetExpression(tac->x), GetExpression(tac->y));
	jumpAddr = tac->z.GetValue();
	return condition;
}

CNode* CBasicBlockTranslator::TranslateTAC(const TAC* tac, size_t& index)
{
	CNode* current = nullptr;
	CNode* expr = nullptr;
	switch (tac->op)
	{
	default:
		return CBasicBlockBaseTranslator::TranslateTAC(tac, index);
	case TACOperator::BOOL_BAND:
		expr = BinaryExpression(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
		return GetNodeFactory().BinaryAssignExprStat(CNodeKind::EXPR_NOT_EQUAL,
			GetExpression(tac->z), expr, GetExpression(TACOperand(0)));
	case TACOperator::BOOL_BIT:
		expr = BinaryExpression(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y));
		return GetNodeFactory().BinaryAssignExprStat(CNodeKind::EXPR_BAND,
			GetExpression(tac->z), expr, GetExpression(TACOperand(1)));
	case TACOperator::CAST:
	{
		auto result = GetExpression(tac->z);
		assert(result->kind == CNodeKind::EXPR_VARIABLE);
		auto type = result->variable->type;  // 要转换到的类型
		expr = GetNodeFactory().Cast(type, GetExpression(tac->x));
		expr = GetNodeFactory().Expr(CNodeKind::EXPR_ASSIGN, result, expr);
		current = GetNodeFactory().ExprStat(expr);
		break;
	}
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
		current = TranslateCall(codes[index], argsNode);
		break;
	}
	case	TACOperator::CALL:
	{
		// 如果有参数，则必是 若干个 ARG 后面跟着一个 CALL
		// 直接出现 CALL，说明没有参数
		current = TranslateCall(tac, nullptr);
		break;
	}

	case TACOperator::IFGEQ:  // 跳转指令是基本块的最后一条指令
		ConditionalJump(CNodeKind::EXPR_GREAT_EQUAL, tac, jumpAddr);
		break;
	case TACOperator::IFGREAT:
		ConditionalJump(CNodeKind::EXPR_GREAT, tac, jumpAddr);
		break;
	case TACOperator::IFEQ:
		ConditionalJump(CNodeKind::EXPR_EQUAL, tac, jumpAddr);
		break;
	case TACOperator::IFNEQ:
		ConditionalJump(CNodeKind::EXPR_NOT_EQUAL, tac, jumpAddr);
		break;
	case TACOperator::IFLESS:
		ConditionalJump(CNodeKind::EXPR_LESS, tac, jumpAddr);
		break;
	case TACOperator::IFLEQ:
		ConditionalJump(CNodeKind::EXPR_LESS_EQUAL, tac, jumpAddr);
		break;
	case TACOperator::IFTRUE:
		condition = GetNodeFactory().Expr(CNodeKind::EXPR_NOT_EQUAL,
			GetExpression(tac->x), GetExpression(TACOperand(0)));
		jumpAddr = tac->z.GetValue();
		break;
	case TACOperator::IFFALSE:
		condition = GetNodeFactory().Expr(CNodeKind::EXPR_EQUAL, GetExpression(tac->x), GetExpression(TACOperand(0)));
		jumpAddr = tac->z.GetValue();
		break;
	case TACOperator::GOTO:
	{
		// 新：当作条件总是真的跳转语句来翻译
		condition = GetNodeFactory().Integer(1);
		jumpAddr = tac->z.GetValue();
		break;

		// 旧： goto 在控制流图中对应一条边，可能被处理成循环结构，也可能就是对应goto语句，
		// 还不知道该怎么处理
		//if (tac->z.GetValue() == tac->address)
		//{
		// // 跳转到自己的语句翻译为 while (1);
		// //expr = allocator.New<CInteger>(1);
		// //current = allocator.New<CWhileStatement>(expr, noneStatement);
		// current = noneStatement;
		// condition = GetNodeFactory().Expr(1);
		// break;
		//}
		//auto label = GetLabelName(tac->z.GetValue());
		//current = GetNodeFactory().Expr(CNodeKind::STAT_GOTO, label);
		break;
	}
	case TACOperator::BIT:
	{
		// 先这样翻译凑合一下，翻译成表达式语句
		expr = GetNodeFactory().Expr(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
		current = GetNodeFactory().ExprStat(expr);
		break;
	}
	case TACOperator::RETURN:
	{
		if (tac->x.IsZero())  // 目前只能返回 AXY 对象，所以可以这么判断有没有返回值
		{
			current = GetNodeFactory().Return();
			break;
		}
		// 有返回值的情况
		current = GetNodeFactory().Return(GetExpression(tac->x));
		break;
	}
	case TACOperator::ROR:
	{
		// C语言中没有ROR运算符，翻译为函数调用好了
		// void Ror(int*, int)
		CNode* listNode = GetNodeFactory().ExprList();
		CListNode list(listNode);
		CNode* params = GetNodeFactory().Expr(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
		list.Add(params);
		list.Add(GetExpression(tac->y));
		current = GetNodeFactory().Call(GetCDB().AddString(_T("Ror")), listNode);
		break;
	}
	case TACOperator::ROL:
	{
		// C语言中没有ROL运算符，翻译为函数调用好了
		// void Rol(int*, int)
		CNode* listNode = GetNodeFactory().ExprList();
		CListNode list(listNode);
		CNode* params = GetNodeFactory().Expr(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
		list.Add(params);
		list.Add(GetExpression(tac->y));
		current = GetNodeFactory().Call(GetCDB().AddString(_T("Rol")), listNode);
		break;
	}
	case TACOperator::PUSH:
	{
		// 还不知道怎么翻译push，先翻译为函数调用吧
		CNode* params = GetExpression(tac->x);
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Push")), params);
		current = GetNodeFactory().Expr(CNodeKind::STAT_EXPR, expr);
		break;
	}
	case TACOperator::POP:
	{
		// 还不知道怎么翻译pop，先翻译为函数调用吧
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Pop")), (CNode*)nullptr);
		current = GetNodeFactory().AssignStat(GetExpression(tac->z), expr);
		break;
	}
	case TACOperator::BOOL_FLAGV:
	{
		// 翻译为函数调用
		CNode* listNode = GetNodeFactory().ExprList();
		CListNode list(listNode);
		list.Add(GetExpression(tac->x));
		list.Add(GetExpression(tac->y));
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("IsOverflow")), (CNode*)nullptr);
		current = GetNodeFactory().AssignStat(GetExpression(tac->z), expr);
		break;
	}

	case TACOperator::CLI:
	{
		// 翻译为函数调用
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Cli")), (CNode*)nullptr);
		current = GetNodeFactory().ExprStat(expr);
		break;
	}
	case TACOperator::SEI:
	{
		// 翻译为函数调用
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Sei")), (CNode*)nullptr);
		current = GetNodeFactory().ExprStat(expr);
		break;
	}
	case TACOperator::CLD:
	{
		// 翻译为函数调用
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Cld")), (CNode*)nullptr);
		current = GetNodeFactory().ExprStat(expr);
		break;
	}
	case TACOperator::SED:
	{
		// 翻译为函数调用
		expr = GetNodeFactory().Call(GetCDB().AddString(_T("Sed")), (CNode*)nullptr);
		current = GetNodeFactory().ExprStat(expr);
		break;
	}
	}
	return current;
}

// 临时变量必定是两条三地址码连着，所以直接合并成一个表达式
CNode* CBasicBlockTranslator::Translate(const TACBasicBlock* block)
{
	CNode* expr = nullptr;
	CNode node(CNodeKind::STAT_LIST);
	CListNode list(node);
	auto& codes = block->GetCodes();
	for (size_t i = 0; i < codes.size(); ++i)
	{
		auto tac = codes[i];
		auto statement = TranslateTAC(tac, i);
		if (statement)
			list.Add(statement);
	}
	Nes::Address firstAddr = codes.empty() ? block->GetStartAddress() : codes[0]->address;
	auto ret = GetTranslator()->NewStatementList(list);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
	GetTranslator()->AddAddressMapStatement(firstAddr, ret);  // 记录下这个基本块对应的地址及语句
	return ret;
}
