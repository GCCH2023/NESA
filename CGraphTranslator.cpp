#include "stdafx.h"
#include "CGraphTranslator.h"
using namespace std;
#include "Function.h"
#include "CDataBase.h"

CGraphTranslator::CGraphTranslator(Allocator& allocator):
CTranslator(allocator),
tempAllocator(1024 * 1024)
{
}


CGraphTranslator::~CGraphTranslator()
{

}

void CGraphTranslator::BuildCFG()
{
	// 首先构造边集
	DirectedGraphEdgeList edges(32);
	edges.clear();
	auto& basicblocks = GetTACFunction()->GetBasicBlocks();
	// 给基本块编号
	for (size_t i = 0; i < basicblocks.size(); ++i)
	{
		basicblocks[i]->tag = (void*)i;
	}
	for (auto block : basicblocks)
	{
		for (auto succ : block->nexts)
		{
			edges.push_back({ (int)block->tag, (int)succ->tag });
		}
	}

	this->graph = std::make_unique<DirectedGraph<ControlTreeNodeEx>>(edges);
}

void CGraphTranslator::Reset()
{
	CTranslator::Reset();

	this->graph.reset();
	tempAllocator.Reset();
}



CNode* CGraphTranslator::ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr)
{
	// 条件跳转指令必定是基本块结束指令
	condition = allocator.New<CNode>(kind, GetExpression(tac->x), GetExpression(tac->y));
	jumpAddr = tac->z.GetValue();
	return condition;
}


CNodeKind CGraphTranslator::TranslateOperator(TACOperator op)
{
	switch (op)
	{
	case	TACOperator::BOR: return CNodeKind::EXPR_BOR;
	case	TACOperator::BAND: return CNodeKind::EXPR_BAND;
	case	TACOperator::ASSIGN: return CNodeKind::EXPR_ASSIGN;
	case	TACOperator::ADD: return CNodeKind::EXPR_ADD;
	case	TACOperator::SUB: return CNodeKind::EXPR_SUB;
	case	TACOperator::XOR: return CNodeKind::EXPR_XOR;
	case TACOperator::SHL: return CNodeKind::EXPR_SHIFT_LEFT;
	case TACOperator::SHR: return CNodeKind::EXPR_SHIFT_RIGHT;
	default:
	{
			   TCHAR buffer[64];
			   _stprintf_s(buffer, _T("三地址码操作码转C表达式：未实现的三地址码操作码 %s"), ToString(op));
			   throw Exception(buffer);
	}
	}
}

CNode* CGraphTranslator::TranslateCall(TAC* call, CNode* params)
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
	CNode* expr = allocator.New<CNode>(name, params);
	// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
	if (call->z.IsTemp())
	{
		expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(call->z), expr);
	}
	return allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
}

// 临时变量必定是两条三地址码连着，所以直接合并成一个表达式
CNode* CGraphTranslator::TranslateRegion(CNode*& pCondition, TACBasicBlock* tacBlock, uint32_t& jumpAddr)
{
	CNode* current = nullptr, *head = nullptr, *tail = nullptr;
	CNode* expr = nullptr;
	auto& codes = tacBlock->GetCodes();
	for (size_t i = 0; i < codes.size(); ++i)
	{
		auto tac = codes[i];
		switch (tac->op)
		{
		case	TACOperator::BOR:
			expr = allocator.New<CNode>(CNodeKind::EXPR_BOR, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::BAND:
			expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::ASSIGN:
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), GetExpression(tac->x));
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::ADD:
			expr = allocator.New<CNode>(CNodeKind::EXPR_ADD, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::SUB:
			expr = allocator.New<CNode>(CNodeKind::EXPR_SUB, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::XOR:
			expr = allocator.New<CNode>(CNodeKind::EXPR_XOR, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::SHL:
			expr = allocator.New<CNode>(CNodeKind::EXPR_SHIFT_LEFT, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::SHR:
			expr = allocator.New<CNode>(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::BOOL_GREAT:
			expr = allocator.New<CNode>(CNodeKind::EXPR_GREAT, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::BOOL_GEQ:
			expr = allocator.New<CNode>(CNodeKind::EXPR_GREAT_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::BOOL_LESS:
			expr = allocator.New<CNode>(CNodeKind::EXPR_LESS, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::BOOL_LEQ:
			expr = allocator.New<CNode>(CNodeKind::EXPR_LESS_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::BOOL_EQ:
			expr = allocator.New<CNode>(CNodeKind::EXPR_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::BOOL_NEQ:
			expr = allocator.New<CNode>(CNodeKind::EXPR_NOT_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::BOOL_BAND:
			expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_NOT_EQUAL, expr, GetExpression(TACOperand(0)));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::BOOL_BIT:
			expr = allocator.New<CNode>(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, expr, GetExpression(TACOperand(1)));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::ARRAY_GET:
		{
									   // 可能是给结构体字段赋值
									   auto x = GetExpression(tac->x);
									   // x 必定是变量
									   assert(x->kind == CNodeKind::EXPR_VARIABLE);
									   auto type = x->variable->type;
									   if (type->GetKind() == TypeKind::Struct)
									   {
										   // y 必定是整数
										   assert(tac->y.IsInterger());
										   // 根据偏移量查找字段
										   auto field = type->GetField(tac->y.GetValue());
										   auto fieldNode = allocator.New<CNode>(field);
										   expr = allocator.New<CNode>(CNodeKind::EXPR_DOT, x, fieldNode);
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
										   expr = allocator.New<CNode>(CNodeKind::EXPR_INDEX, x, GetExpression(tac->y));
									   }
									   expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
									   current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
									   break;

		}
		case TACOperator::ARRAY_SET:
		{
									   // 可能是给结构体字段赋值
									   auto x = GetExpression(tac->x);
									   // x 必定是变量
									   assert(x->kind == CNodeKind::EXPR_VARIABLE);
									   auto type = x->variable->type;
									   if (type->GetKind() == TypeKind::Struct)
									   {
										   // y 必定是整数
										   assert(tac->y.IsInterger());
										   // 根据偏移量查找字段
										   auto field = type->GetField(tac->y.GetValue());
										   auto fieldNode = allocator.New<CNode>(field);
										   expr = allocator.New<CNode>(CNodeKind::EXPR_DOT, x, fieldNode);
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
										   expr = allocator.New<CNode>(CNodeKind::EXPR_INDEX, x, GetExpression(tac->y));
									   }
									   expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, expr, GetExpression(tac->z));
									   current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
									   break;
		}
		case TACOperator::ADDR:
			expr = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::DEREF:
			expr = allocator.New<CNode>(CNodeKind::EXPR_DEREF, GetExpression(tac->x));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::CAST:
		{
								  auto result = GetExpression(tac->z);
								  assert(result->kind == CNodeKind::EXPR_VARIABLE);
								  auto type = result->variable->type;  // 要转换到的类型
								  expr = allocator.New<CNode>(type, GetExpression(tac->x));
								  expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, result, expr);
								  current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								  break;
		}
		case	TACOperator::ARG:
		{
									// 若干个 ARG 后面跟着一个 CALL
									// 遇到 ARG，则要连着后面的直到 CALL 的三地址码一起翻译
									CNode* params = nullptr;
									CNode* paramsTail = nullptr;
									while (codes[i]->op == TACOperator::ARG)
									{
										if (!paramsTail)
										{
											paramsTail = params = GetExpression(codes[i]->x);
										}
										else
										{
											paramsTail->next = GetExpression(codes[i]->x);
											paramsTail = paramsTail->next;
										}
										++i;
									}
									if (codes[i]->op != TACOperator::CALL)
										throw Exception(_T("三地址码翻译为C语句：ARG 后面不是 CALL"));
									// 最后是 CALL 指令
									current = TranslateCall(codes[i]);
									break;
		}
		case	TACOperator::CALL:
		{
									 // 如果有参数，则必是 若干个 ARG 后面跟着一个 CALL
									 // 直接出现 CALL，说明没有参数
									 current = TranslateCall(tac);
									 break;
		}

		case TACOperator::IFGEQ:  // 跳转指令是基本块的最后一条指令
			ConditionalJump(pCondition, CNodeKind::EXPR_GREAT_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFGREAT:
			ConditionalJump(pCondition, CNodeKind::EXPR_GREAT, tac, jumpAddr);
			continue;
		case TACOperator::IFEQ:
			ConditionalJump(pCondition, CNodeKind::EXPR_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFNEQ:
			ConditionalJump(pCondition, CNodeKind::EXPR_NOT_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFLESS:
			ConditionalJump(pCondition, CNodeKind::EXPR_LESS, tac, jumpAddr);
			continue;
		case TACOperator::IFLEQ:
			ConditionalJump(pCondition, CNodeKind::EXPR_LESS_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFTRUE:
			pCondition = allocator.New<CNode>(CNodeKind::EXPR_NOT_EQUAL,
				GetExpression(tac->x), GetExpression(TACOperand(0)));
			jumpAddr = tac->z.GetValue();
			continue;
		case TACOperator::IFFALSE:
			pCondition = allocator.New<CNode>(CNodeKind::EXPR_EQUAL, GetExpression(tac->x), GetExpression(TACOperand(0)));
			jumpAddr = tac->z.GetValue();
			continue;
		case TACOperator::GOTO:
		{
								  // 新：当作条件总是真的跳转语句来翻译
								  pCondition = allocator.New<CNode>(1);
								  jumpAddr = tac->z.GetValue();
								  continue;

								  // 旧： goto 在控制流图中对应一条边，可能被处理成循环结构，也可能就是对应goto语句，
								  // 还不知道该怎么处理
								  //if (tac->z.GetValue() == tac->address)
								  //{
								  // // 跳转到自己的语句翻译为 while (1);
								  // //expr = allocator.New<CInteger>(1);
								  // //current = allocator.New<CWhileStatement>(expr, noneStatement);
								  // current = noneStatement;
								  // pCondition = allocator.New<CNode>(1);
								  // break;
								  //}
								  //auto label = GetLabelName(tac->z.GetValue());
								  //current = allocator.New<CNode>(CNodeKind::STAT_GOTO, label);
								  break;
		}
		case TACOperator::BIT:
		{
								 // 先这样翻译凑合一下，翻译成表达式语句
								 expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
		case TACOperator::RETURN:
		{
									if (tac->x.IsZero())  // 目前只能返回 AXY 对象，所以可以这么判断有没有返回值
									{
										current = allocator.New<CNode>(CNodeKind::STAT_RETURN);
										break;
									}
									// 有返回值的情况
									current = allocator.New<CNode>(CNodeKind::STAT_RETURN, GetExpression(tac->x));
									break;
		}
		case TACOperator::ROR:
		{
								 // C语言中没有ROR运算符，翻译为函数调用好了
								 // void Ror(int*, int)
								 CNode* params = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
								 params->next = GetExpression(tac->y);
								 current = allocator.New<CNode>(GetCDB().AddString(_T("Ror")), params);
								 break;
		}
		case TACOperator::ROL:
		{
								 // C语言中没有ROL运算符，翻译为函数调用好了
								 // void Rol(int*, int)
								 CNode* params = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
								 params->next = GetExpression(tac->y);
								 current = allocator.New<CNode>(GetCDB().AddString(_T("Rol")), params);
								 break;
		}
		case TACOperator::PUSH:
		{
								  // 还不知道怎么翻译push，先翻译为函数调用吧
								  CNode* params = GetExpression(tac->x);
								  expr = allocator.New<CNode>(GetCDB().AddString(_T("Push")), params);
								  current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								  break;
		}
		case TACOperator::POP:
		{
								 // 还不知道怎么翻译pop，先翻译为函数调用吧
								 expr = allocator.New<CNode>(GetCDB().AddString(_T("Pop")), (CNode*)nullptr);
								 expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
		case TACOperator::BOOL_FLAGV:
		{
								   // 翻译为函数调用
								   CNode* params = GetExpression(tac->x);
								   params->next = GetExpression(tac->y);
								   expr = allocator.New<CNode>(GetCDB().AddString(_T("IsOverflow")), (CNode*)nullptr);
								   expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
								   current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								   break;
		}

		case TACOperator::CLI:
		{
								 // 翻译为函数调用
								 expr = allocator.New<CNode>(GetCDB().AddString(_T("Cli")), (CNode*)nullptr);
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
		case TACOperator::SEI:
		{
								 // 翻译为函数调用
								 expr = allocator.New<CNode>(GetCDB().AddString(_T("Sei")), (CNode*)nullptr);
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
		case TACOperator::CLD:
		{
								 // 翻译为函数调用
								 expr = allocator.New<CNode>(GetCDB().AddString(_T("Cld")), (CNode*)nullptr);
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
		case TACOperator::SED:
		{
								 // 翻译为函数调用
								 expr = allocator.New<CNode>(GetCDB().AddString(_T("Sed")), (CNode*)nullptr);
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
			//case TACOperator::CLC:
			//case TACOperator::SEC:
			//case TACOperator::CLV:
			// 进位和溢出标志都是和其他指令配合使用的，抽象语法树中不应该出现
		default:
		{
				   TCHAR buffer[64];
				   _stprintf_s(buffer, _T("三地址码转C语句：未实现的三地址码 %s"), ToString(tac->op));
				   throw Exception(buffer);
		}
		}
		// 构建语句列表
		if (!head)
		{
			head = tail = current;
		}
		else
		{
			tail->next = current;
			tail = current;
		}
	}
	Nes::Address firstAddr = codes.empty() ? tacBlock->GetStartAddress() : codes[0]->address;
	auto ret = NewStatementList(head, tail);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
	AddAddressMapStatement(firstAddr, ret);  // 记录下这个基本块对应的地址及语句
	return ret;
}


CNode* CGraphTranslator::CombineListIf(CNode* statement, CNode* condition, CNode* body, CNode* elseBody /*= nullptr*/)
{
	auto ifStat = allocator.New<CNode>(CNodeKind::STAT_IF, condition, body, elseBody);
	if (!statement)
		return ifStat;
	return NewStatementPair(statement, ifStat);
}

CNode* CGraphTranslator::GetNotExpression(CNode* expr)
{
	switch (expr->kind)
	{
	case CNodeKind::EXPR_GREAT:
		expr->kind = CNodeKind::EXPR_LESS_EQUAL;
		break;
	case CNodeKind::EXPR_GREAT_EQUAL:
		expr->kind = CNodeKind::EXPR_LESS;
		break;
	case CNodeKind::EXPR_LESS:
		expr->kind = CNodeKind::EXPR_GREAT_EQUAL;
		break;
	case CNodeKind::EXPR_LESS_EQUAL:
		expr->kind = CNodeKind::EXPR_GREAT;
		break;
	case CNodeKind::EXPR_EQUAL:
		expr->kind = CNodeKind::EXPR_NOT_EQUAL;
		break;
	case CNodeKind::EXPR_NOT_EQUAL:
		expr->kind = CNodeKind::EXPR_EQUAL;
		break;
	default:
		throw Exception(_T("未实现的表达式取反类型"));
	}
	return expr;
}



CNode* CGraphTranslator::NewDoWhile(CNode* condition, CNode* body)
{
	// do ; while (condition) => while (condition) ;
	// 没有循环体或者条件总是为真，则转换为 while 循环
	if (body->kind == CNodeKind::STAT_NONE || condition->kind == CNodeKind::EXPR_INTEGER)
		return allocator.New<CNode>(CNodeKind::STAT_WHILE, condition, body);
	return allocator.New<CNode>(CNodeKind::STAT_DO_WHILE, condition, body);
}


CNode* CGraphTranslator::NewStatementPair(CNode* first, CNode* second)
{
	assert(first->next == nullptr);
	// 尝试优化
	// 在这里优化，可能有一个问题：有的地方可能引用了其中一个指针
	// 合并后，被丢弃了，引用失效。
	// 但是这个问题也不算是问题，因为区域归约后，子区域一般不访问了
	// 发现了新的问题，把标签语句给优化掉了，还是生成语句后再优化好了
	//if (first->kind == CNodeKind::STAT_LIST)
	//{
	//	if (second->kind == CNodeKind::STAT_LIST)
	//	{
	//		// 合并到末尾
	//		first->list.tail->next = second->list.head;
	//		first->list.tail = second->list.tail;
	//		return first;
	//	}
	//	// 添加到末尾
	//	first->list.tail->next = second;
	//	first->list.tail = second;
	//	return first;
	//}
	//else if (second->kind == CNodeKind::STAT_LIST)
	//{
	//	// 添加到开头
	//	first->next = second->list.head;
	//	second->list.head = first;
	//	return second;
	//}
	first->next = second;
	return NewStatementList(first, second);
}




// 当要将控制流图中的一个自循环节点归约时
// a -> a
void CGraphTranslator::OnReduceSelfLoop(Node n)
{
	auto node = graph->GetNode(n);
	if (node->tag.type != CTNTYPE_LEAF)
	{
		if (node->tag.condition == nullptr)
			throw Exception(_T("非叶子自循环节点异常"));
		node->tag.statement = NewDoWhile(node->tag.condition, node->tag.statement);
		return;
	}
	CNode* condition = nullptr;
	auto block = this->GetTACFunction()->GetBasicBlocks()[node->index];
	uint32_t jumpAddr;
	auto a = TranslateRegion(condition, block, jumpAddr);
	node->tag.statement = NewDoWhile(condition, a);
}

void CGraphTranslator::OnReduceList(Node f, Node s)
{
	auto first = graph->GetNode(f);
	auto second = graph->GetNode(s);
	CNode* condition = nullptr;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	if (first->tag.type == CTNTYPE_LEAF)
	{
		first->tag.statement = TranslateRegion(condition, blocks[first->index], jumpAddr);
	}
	if (second->tag.type == CTNTYPE_LEAF)
	{
		second->tag.statement = TranslateRegion(condition, blocks[second->index], jumpAddr);
	}
	first->tag.statement = NewStatementPair(first->tag.statement, second->tag.statement);
	first->tag.condition = condition;
}

void CGraphTranslator::OnReducePoint2Loop(Node f, Node s)
{
	auto first = graph->GetNode(f);
	auto second = graph->GetNode(s);
	auto node = first;  // 结果
	CNode* condition = nullptr;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	if (first->tag.type == CTNTYPE_LEAF)
	{
		first->tag.statement = TranslateRegion(condition, blocks[first->index], jumpAddr);
		// 跳转边翻译为 goto 语句
		auto gotoStat = allocator.New<CNode>(CNodeKind::STAT_GOTO, GetLabelName(jumpAddr));
		first->tag.statement = CombineListIf(first->tag.statement, condition, gotoStat);
	}
	else
	{
		throw Exception(_T("2点循环的第一个节点不是叶子节点的归约未实现"));
	}
	if (second->tag.type == CTNTYPE_LEAF)
	{
		second->tag.statement = TranslateRegion(condition, blocks[second->index], jumpAddr);
	}
	else
	{
		assert(second->tag.statement);
		assert(second->tag.condition);
		condition = second->tag.condition;
	}
	node->tag.statement = NewStatementPair(first->tag.statement, second->tag.statement);
	node->tag.statement = NewDoWhile(condition, node->tag.statement);
}

void CGraphTranslator::OnReduceIf(Node _if, Node then)
{
	auto cond = graph->GetNode(_if);
	auto body = graph->GetNode(then);
	auto node = cond;  // 结果
	CNode* condition = nullptr;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond->tag.type == CTNTYPE_LEAF)
	{
		cond->tag.statement = TranslateRegion(condition, blocks[cond->index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!cond->tag.condition)
			throw Exception(_T("翻译为 if 语句的过程中缺少 if 语句的条件表达式"));
		condition = cond->tag.condition;
	}
	CNode* ifCond = condition;
	if (body->tag.type == CTNTYPE_LEAF)
	{
		body->tag.statement = TranslateRegion(condition, blocks[body->index], jumpAddr);
	}
	// 在 if 语句之前还有一段代码
	node->tag.statement = CombineListIf(cond->tag.statement, ifCond, body->tag.statement);
}

void CGraphTranslator::OnReduceIfElse(Node _if, Node t, Node e)
{
	auto cond = graph->GetNode(_if);
	auto then = graph->GetNode(t);
	auto _else = graph->GetNode(e);
	auto node = cond;  // 结果
	CNode* condition = nullptr;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond->tag.type == CTNTYPE_LEAF)
	{
		cond->tag.statement = TranslateRegion(condition, blocks[cond->index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!cond->tag.condition)
			throw Exception(_T("翻译为 if - else 语句的过程中缺少 if 语句的条件表达式"));
		condition = cond->tag.condition;
	}
	CNode* ifCond = condition;
	// 需要根据跳转地址来判断哪个基本块是 then 部分，哪个是 else 部分
	if (jumpAddr == blocks[then->index]->GetStartAddress())
	{
		// 这种情况，需要交换 then 和 else 部分
		std::swap(node->tag._if.then, node->tag._if._else);
		std::swap(then, _else);
	}
	if (then->tag.type == CTNTYPE_LEAF)
	{
		then->tag.statement = TranslateRegion(condition, blocks[then->index], jumpAddr);
	}
	if (_else->tag.type == CTNTYPE_LEAF)
	{
		_else->tag.statement = TranslateRegion(condition, blocks[_else->index], jumpAddr);
	}
	// 在 if 语句之前还有一段代码
	node->tag.statement = CombineListIf(cond->tag.statement, ifCond, then->tag.statement, _else->tag.statement);
}

// a -> b, a -> c, b ->c, b -> d, c -> d 翻译为 if (x || y) { c }
// 其中 a 包含 条件 x，b 包含条件 y， c是条件满足时要执行的
void CGraphTranslator::OnReduceIfOr(Node _if, Node then, Node _else)
{
	auto a = graph->GetNode(_if);
	auto b = graph->GetNode(then);
	auto c = graph->GetNode(_else);
	auto node = a;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	CNode* condition1 = nullptr, *condition2 = nullptr;
	if (a->tag.type == CTNTYPE_LEAF)
	{
		a->tag.statement = TranslateRegion(condition1, blocks[a->index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!a->tag.condition)
			throw Exception(_T("翻译为 if - or 语句的过程中缺少 if 语句的第1个条件表达式"));
		condition1 = a->tag.condition;
	}
	// 需要根据跳转地址来判断哪个基本块是 then 部分，哪个是 else 部分
	if (jumpAddr == blocks[b->index]->GetStartAddress())
	{
		// 这种情况，需要交换 then 和 else 部分
		std::swap(node->tag._if.then, node->tag._if._else);
		std::swap(b, c);
	}
	if (b->tag.type == CTNTYPE_LEAF)
	{
		b->tag.statement = TranslateRegion(condition2, blocks[b->index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!b->tag.condition)
			throw Exception(_T("翻译为 if 语句的过程中缺少 if 语句的第2个条件表达式"));
		condition2 = b->tag.condition;
	}
	// 用 || 连接 a 和 b 的条件，b的条件要取反，因为b条件满足时跳转到d
	condition2 = GetNotExpression(condition2);
	condition1 = allocator.New<CNode>(CNodeKind::EXPR_OR, condition1, condition2);
	if (c->tag.type == CTNTYPE_LEAF)
	{
		c->tag.statement = TranslateRegion(condition2, blocks[c->index], jumpAddr);
	}

	// 多出的那条边翻译为 goto 语句，多出的边的尾节点只有一个后继，所以不会给condition赋值
	//auto name = GetLabelName(blocks[c->index]->tag.GetStartAddress());
	//auto label = allocator.New<CLabelStatement>(name.c_str(), c->tag.statement);  // 尾节点的语句替换为标签语句
	//c->tag.statement = label;

	//auto gotoStat = allocator.New<CGotoStatement>(label);
	//b->tag.statement = CombineListIf(b->tag.statement, condition, b->tag.statement, c->tag.statement);  // 头节点的末尾加上一个条件跳转语句

	// 在 if 语句之前还有一段代码
	node->tag.statement = CombineListIf(a->tag.statement, condition1, c->tag.statement);
}



Node CGraphTranslator::CReduce(Node parent, vector<Node> children, CtrlTreeNodeType type)
{
	auto ctNode = graph->GetNode(parent);
	switch (type)
	{
	case CTNTYPE_LEAF:
		throw Exception(_T("不能将区域归约为叶子区域"));
		break;
	}
	ctNode->tag.type = type;
	ctNode->index = parent;

	//COUT << "归约 " << ToString(type) << " " << parent << " : ";
	//for (auto n : children)
	//	COUT << n << ", ";
	//COUT << endl;
	//ctNode->Dump();
	//COUT << ctNode->statement;
	//COUT << endl;
	return parent;
}

Node CGraphTranslator::ReduceRegionList(NodeSet& N, Node a, Node b)
{
	// r 的前驱是 a 的前驱

	// r 的后继是 b 的后继
	graph->GetNode(a)->succ = graph->GetNode(b)->succ;
	for (auto s : graph->GetNode(b)->Succ())
		graph->GetNode(s)->pred.Replace(b, a);

	// 使用 r 代替 a, b
	N -= b;

	OnReduceList(a, b);
	graph->GetNode(a)->tag.type = CTNTYPE_LIST;

	return CReduce(a, { a, b }, CTNTYPE_LIST);
}

Node CGraphTranslator::ReduceRegionSelfLoop(NodeSet& N, Node a)
{
	// r 的前驱是 a 除了 a 之外的前驱
	graph->GetNode(a)->pred -= a;

	// r 的后继是 a 除了 a 之外的后继
	graph->GetNode(a)->succ -= a;

	// 使用 r 代替 a
	OnReduceSelfLoop(a);
	graph->GetNode(a)->tag.type = CTNTYPE_SELF_LOOP;

	return CReduce(a, { a }, CTNTYPE_SELF_LOOP);
}

Node CGraphTranslator::ReduceRegionIfElse(NodeSet& N, Node a, Node b, Node c)
{
	Node r = a;
	// r 的前驱是 a 的前驱

	// r 的后继是 b 和 c 的后继，且 b 和 c 都只有一个相同的后继
	graph->GetNode(r)->succ = graph->GetNode(b)->succ;
	for (auto s : graph->GetNode(b)->Succ())
	{
		graph->GetNode(s)->pred.Replace({ b, c }, { r });
	}

	// 使用 r 代替 a, b, c
	N -= b;
	N -= c;
	OnReduceIfElse(a, b, c);
	graph->GetNode(r)->tag.type = CTNTYPE_IF_ELSE;

	return CReduce(r, { a, b, c }, CTNTYPE_IF_ELSE);
}

Node CGraphTranslator::ReduceRegionIfOr(NodeSet& N, Node a, Node b, Node c)
{
	Node r = a;
	// r 的前驱是 a 的前驱

	// r 的后继是 b 和 c 的后继 d，且 b -> c, b -> d, c -> d
	graph->GetNode(r)->succ = graph->GetNode(c)->succ;
	for (auto s : graph->GetNode(r)->Succ())
	{
		graph->GetNode(s)->pred.Replace({ b, c }, { r });
	}

	// 使用 r 代替 a, b, c
	N -= b;
	N -= c;

	OnReduceIfOr(a, b, c);
	graph->GetNode(r)->tag.type = CTNTYPE_IF_OR;

	return CReduce(r, { a, b, c }, CTNTYPE_IF_OR);
}

Node CGraphTranslator::ReduceRegionIf(NodeSet& N, Node a, Node b)
{
	Node r = a;
	// r 的前驱是 a 的前驱

	// r 的后继是 a 和 b 的后继，a 只有 b, c 两个后继，b 只有 c 一个后继
	graph->GetNode(r)->succ = graph->GetNode(b)->succ;
	for (auto s : graph->GetNode(b)->Succ())
	{
		graph->GetNode(s)->pred.Replace({ a, b }, { r });
	}

	// 使用 r 代替 a, b
	N -= b;

	OnReduceIf(a, b);
	graph->GetNode(r)->tag.type = CTNTYPE_IF;

	return CReduce(r, { a, b }, CTNTYPE_IF);
}

Node CGraphTranslator::ReduceRegionPoint2Loop(NodeSet& N, Node a, Node b)
{
	Node r = a;
	// r 的前驱是 a 除了 b 之外的前驱
	graph->GetNode(r)->pred = graph->GetNode(a)->pred - b;

	// a 的除 b 之外的后继翻译为 goto 语句
	for (auto s : graph->GetNode(a)->Succ())
	{
		if (s == b)
			continue;
		// a goto s 这条边的goto应该是叶子节点到叶子节点的边
		// COUT << a << " goto " << s << endl;
		// 移除这条边
		graph->GetNode(a)->succ -= s;
		graph->GetNode(s)->pred -= a;
		// GetLeafEdges({ a, s });
		/*SetSub(graph->GetNode(s)->pred, a);
		SetUnion(graph->GetNode(s)->pred, r);*/
	}

	// r 的后继是 b 除了 a 之外的后继
	graph->GetNode(r)->succ = graph->GetNode(b)->succ - a;
	for (auto s : graph->GetNode(r)->Succ())
		graph->GetNode(s)->pred.Replace(b, r);

	// 替换 a，b 为 r
	N -= b;

	OnReducePoint2Loop(a, b);

	return CReduce(r, { a, b }, CTNTYPE_P2LOOP);
}

NodeSet CGraphTranslator::CAnalysis(NodeSet N)
{
	while (true)
	{
		for (auto n : Nodes(N))
		{
			switch (graph->GetNode(n)->GetSuccCount())
			{
			case 1:
			{
					  Node succ = graph->GetNode(n)->Succ()[0];
					  if (graph->GetNode(succ)->GetPredCount() == 1)
					  {
						  if (succ == n)
						  {
							  ReduceRegionSelfLoop(N, n);
							  goto NEXT;
						  }
						  ReduceRegionList(N, n, succ);
						  // 下一次循环
						  goto NEXT;
					  }
					  break;
			}
			case 2:
			{
					  auto succ = graph->GetNode(n)->Succ();
					  auto b = graph->GetNode(succ[0]);
					  auto c = graph->GetNode(succ[1]);
					  // a -> b, a -> c, b -> d, c -> d 归约为 if else 结构
					  if (b->succ == c->succ && b->GetSuccCount() == 1 &&
						  b->GetPredCount() == 1 && c->GetPredCount() == 1)
					  {
						  ReduceRegionIfElse(N, n, b->index, c->index);
						  goto NEXT;
					  }
					  // a -> b, a -> c, b -> d, c -> d, b -> c 归约为 if (x || y)
					  if ((b->succ & c->succ) != 0)  // b 和 c 有相同的后继
					  {
						  if (b->GetSuccCount() == 2 && c->GetSuccCount() == 1 && b->succ.Contains(c->index))
						  {
							  // b -> c 的边翻译为 goto
							  ReduceRegionIfOr(N, n, b->index, c->index);
							  goto NEXT;
						  }
						  if (c->GetSuccCount() == 2 && b->GetSuccCount() == 1 && c->succ.Contains(b->index))
						  {
							  ReduceRegionIfOr(N, n, c->index, b->index);
							  // c -> b 的边翻译为 goto
							  goto NEXT;
						  }
					  }

					  if (b->GetPredCount() == 1 && b->GetSuccCount() == 1 &&
						  b->succ.Contains(c->index))
					  {
						  ReduceRegionIf(N, n, b->index);
						  //DumpCurrentCFG(N);
						  goto NEXT;
					  }
					  if (c->GetPredCount() == 1 && c->GetSuccCount() == 1 &&
						  c->succ.Contains(b->index))
					  {
						  ReduceRegionIf(N, n, c->index);
						  //DumpCurrentCFG(N);
						  goto NEXT;
					  }
					  // 检测 if

					  break;
			}
			}
			// 循环检测
			for (auto s : graph->GetNode(n)->Succ())
			{
				if (s == n)  // 自循环检测
				{
					//DumpCurrentCFG(N);
					ReduceRegionSelfLoop(N, n);
					//DumpCurrentCFG(N);
					goto NEXT;
				}
				// 两点循环 a -> b && b -> a 并且 b 只有一个前驱
				if (graph->GetNode(s)->succ.Contains(n) && graph->GetNode(s)->GetPredCount() == 1)
				{
					ReduceRegionPoint2Loop(N, n, s);
					goto NEXT;
				}
			}
		}
		break;
	NEXT:
		// DumpCurrentCFG(N);
		;
	}
	return N;
}

CNode* CGraphTranslator::TranslateBody()
{
	BuildCFG();

	NodeSet N;
	try
	{
		N = this->graph->GetFullSet();
		//DumpCurrentCFG(N);
		N = CAnalysis(N);
	}
	catch (Exception& e)
	{
		COUT << e.Message() << std::endl;
	}
	if (N.Count() != 1)  // 也可能只有一个基本块
	{
		// DumpCurrentCFG(N);
		Sprintf<> s;
		s.Format(_T("翻译 %04X 时，控制树无法归约到单一根节点"), GetTACFunction()->GetStartAddress());
		throw Exception(s.ToString());
	}
	// 在全部语句都生成后，回填标签语句
	PatchLabels();

	Node n = N.ToVector()[0];
	if (graph->GetNode(n)->tag.statement == nullptr)
	{
		CNode* condition = nullptr;
		uint32_t jumpAddr;
		auto block = GetTACFunction()->GetBasicBlocks()[n];
		graph->GetNode(n)->tag.statement = TranslateRegion(condition, block, jumpAddr);
	}
	return graph->GetNode(n)->tag.statement;
}

void CGraphTranslator::DumpControlTree()
{
	for (int i = 0; i < graph->GetNodeCount(); ++i)
	{
		auto block = graph->GetNode(i);
		COUT << _T("tree node ") << i << _T(" , 前驱 : ");
		DumpNodeSet(block->pred);
		COUT << _T(" 后继 : ");
		DumpNodeSet(block->succ);
		COUT << endl;
	}
}

void CGraphTranslator::DumpCurrentCFG(NodeSet& N)
{
	auto nodes = Nodes(N);
	for (auto i : nodes)
	{
		auto node = graph->GetNode(i);
		COUT << _T("tree node ") << i << _T(" , 前驱 : ");
		DumpNodeSet(node->pred);
		COUT << _T(" 后继 : ");
		DumpNodeSet(node->succ);
		COUT << endl;
	}
}
