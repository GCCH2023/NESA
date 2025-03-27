#include "stdafx.h"
#include "CDirectTranslator.h"
#include "Function.h"
#include "TACFunction.h"
#include "TypeManager.h"
#include "CDataBase.h"

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
	CNode* expr = allocator.New<CNode>(name, params);
	// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
	if (call->z.IsTemp())
	{
		expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(call->z), expr);
	}
	return allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
}

CNode* CDirectTranslator::ConditionalJump(CNodeKind kind, TAC* tac)
{
	// 条件跳转指令必定是基本块结束指令
	auto condition = allocator.New<CNode>(kind, GetExpression(tac->x), GetExpression(tac->y));
	auto jumpAddr = tac->z.GetValue();
	auto gotoStat = allocator.New<CNode>(CNodeKind::STAT_GOTO, GetLabelName(jumpAddr));
	return allocator.New<CNode>(kind, condition, gotoStat);
}


CNode* CDirectTranslator::TranslateBody()
{
	CNode* expr = nullptr;
	CNode* current = nullptr, *head = nullptr, *tail = nullptr;
	auto& codes = GetTACFunction()->GetCodes();
	// 遍历所有指令，直接翻译为C语句
	for (size_t i = 0; i < codes.size();++i)
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
			current = ConditionalJump(CNodeKind::EXPR_GREAT_EQUAL, tac);
			break;
		case TACOperator::IFGREAT:
			current = ConditionalJump(CNodeKind::EXPR_GREAT, tac);
			break;
		case TACOperator::IFEQ:
			current = ConditionalJump(CNodeKind::EXPR_EQUAL, tac);
			break;
		case TACOperator::IFNEQ:
			current = ConditionalJump(CNodeKind::EXPR_NOT_EQUAL, tac);
			break;
		case TACOperator::IFLESS:
			current = ConditionalJump(CNodeKind::EXPR_LESS, tac);
			break;
		case TACOperator::IFLEQ:
			current = ConditionalJump(CNodeKind::EXPR_LESS_EQUAL, tac);
			break;
		case TACOperator::IFTRUE:
			tac->y = 0;
			current = ConditionalJump(CNodeKind::EXPR_NOT_EQUAL, tac);
			break;
		case TACOperator::IFFALSE:
			tac->y = 0;
			current = ConditionalJump(CNodeKind::EXPR_EQUAL, tac);
			break;
		case TACOperator::GOTO:
			current = allocator.New<CNode>(CNodeKind::STAT_GOTO, GetLabelName(tac->z.GetValue()));
			break;
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
	return head;
}
