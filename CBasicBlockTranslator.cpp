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
	}
	return current;
}

// 临时变量必定是两条三地址码连着，所以直接合并成一个表达式
CNode* CBasicBlockTranslator::OnTranslate(const TACBasicBlock* block)
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
