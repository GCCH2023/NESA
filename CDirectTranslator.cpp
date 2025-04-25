#include "stdafx.h"
#include "CDirectTranslator.h"
#include "Function.h"
#include "TACFunction.h"
#include "TypeManager.h"
#include "CDataBase.h"
#include "CBasicBlockTranslator.h"

CDirectTranslator::CDirectTranslator(Allocator& allocator) :
CTranslator(allocator)
{
}


CDirectTranslator::~CDirectTranslator()
{
}

void CDirectTranslator::Reset()
{
	CTranslator::Reset();
}

CNode* CDirectTranslator::TranslateCall(TAC* call, CNode* params)
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
		name = GetLocalVariableName(call->x.GetValue());
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
	CNode* expr = nodeFactory.Call(name, params);
	// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
	if (call->z.IsTemp())
	{
		expr = nodeFactory.Expr(CNodeKind::EXPR_ASSIGN, GetExpression(call->z), expr);
	}
	return nodeFactory.ExprStat(expr);
}

CNode* CDirectTranslator::ConditionalJump(CNodeKind kind, TAC* tac)
{
	assert(IsExpression(kind));
	// 条件跳转指令必定是基本块结束指令
	auto condition = nodeFactory.Expr(kind, GetExpression(tac->x), GetExpression(tac->y));
	auto jumpAddr = tac->z.GetValue();
	auto gotoStat = nodeFactory.Goto(GetLabelName(jumpAddr));
	return nodeFactory.If(condition, gotoStat);
}

CNode* CDirectTranslator::ConditionalJump(const BasicBlockResult& ret)
{
	if (!ret.condition)
		return GetNodeFactory().EmptyStat();

	auto gotoStat = nodeFactory.Goto(GetLabelName(ret.jumpTarget));
	return nodeFactory.If(ret.condition, gotoStat);
}

CNode* CDirectTranslator::UnaryExpression(CNodeKind kind, const TAC* tac)
{
	return nodeFactory.UnaryAssignExprStat(kind, GetExpression(tac->z), GetExpression(tac->x));
}

CNode* CDirectTranslator::BinaryExpression(CNodeKind kind, const TAC* tac)
{
	return nodeFactory.BinaryAssignExprStat(kind, GetExpression(tac->z), GetExpression(tac->x), GetExpression(tac->y));
}


CNode* CDirectTranslator::TranslateBody()
{
	auto& blocks = GetTACFunction()->GetBasicBlocks();
	CNode funcNode(CNodeKind::STAT_LIST);
	CListNode funcList(funcNode);
	for (auto block : blocks)
	{
		CNode node(CNodeKind::STAT_LIST);
		CListNode list(node);
		auto ret = TranslateBasicBlock(block);
		list.Add(ret.statement);
		list.Add(ConditionalJump(ret));
		auto blockStat = NewStatementList(list);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
		AddAddressMapStatement(block->GetStartAddress(), blockStat);
		funcList.Add(blockStat);
	}
	auto funcHead = NewStatementList(funcList);
	// 在全部语句都生成后，回填标签语句
	PatchLabels();

	return funcHead;
}

BasicBlockResult CDirectTranslator::TranslateBasicBlock(const TACBasicBlock* block)
{
	CBasicBlockTranslator translator(this);
	BasicBlockResult ret;
	ret.statement = translator.Translate(block);
	ret.condition = translator.GetCondition();
	ret.jumpTarget = translator.GetJumpTarget();
	return ret;
}
