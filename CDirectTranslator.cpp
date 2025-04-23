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
		auto& codes = block->GetCodes();
		CNode* expr = nullptr;
		CNode* current = nullptr;
		CNode node(CNodeKind::STAT_LIST);
		CListNode list(node);
		// 遍历所有指令，直接翻译为C语句
		for (size_t i = 0; i < codes.size(); ++i)
		{
			auto tac = codes[i];
			switch (tac->op)
			{
			case	TACOperator::BOR:
				current = BinaryExpression(CNodeKind::EXPR_BOR, tac);
				break;
			case	TACOperator::BAND:
				current = BinaryExpression(CNodeKind::EXPR_BAND, tac);
				break;
			case	TACOperator::ADD:
				current = BinaryExpression(CNodeKind::EXPR_ADD, tac);
				break;
			case	TACOperator::SUB:
				current = BinaryExpression(CNodeKind::EXPR_SUB, tac);
				break;
			case	TACOperator::XOR:
				current = BinaryExpression(CNodeKind::EXPR_XOR, tac);
				break;
			case	TACOperator::SHL:
				current = BinaryExpression(CNodeKind::EXPR_SHIFT_LEFT, tac);
				break;
			case	TACOperator::SHR:
				current = BinaryExpression(CNodeKind::EXPR_SHIFT_RIGHT, tac);
				break;
			case TACOperator::BOOL_GREAT:
				current = BinaryExpression(CNodeKind::EXPR_GREAT, tac);
				break;
			case TACOperator::BOOL_GEQ:
				current = BinaryExpression(CNodeKind::EXPR_GREAT_EQUAL, tac);
				break;
			case TACOperator::BOOL_LESS:
				current = BinaryExpression(CNodeKind::EXPR_LESS, tac);
				break;
			case TACOperator::BOOL_LEQ:
				current = BinaryExpression(CNodeKind::EXPR_LESS_EQUAL, tac);
				break;
			case TACOperator::BOOL_EQ:
				current = BinaryExpression(CNodeKind::EXPR_EQUAL, tac);
				break;
			case TACOperator::BOOL_NEQ:
				current = BinaryExpression(CNodeKind::EXPR_NOT_EQUAL, tac);
				break;
			case TACOperator::BOOL_BAND:
				expr = GetNodeFactory().Expr(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
				current = GetNodeFactory().BinaryAssignExprStat(CNodeKind::EXPR_NOT_EQUAL,
					GetExpression(tac->z), expr, GetExpression(TACOperand(0)));
				break;
			case TACOperator::BOOL_BIT:
				expr = GetNodeFactory().Expr(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y));
				current = GetNodeFactory().BinaryAssignExprStat(CNodeKind::EXPR_BAND,
					GetExpression(tac->z), expr, GetExpression(TACOperand(1)));
				break;
			case	TACOperator::ASSIGN:
				current = GetNodeFactory().AssignStat(GetExpression(tac->z), GetExpression(tac->x));
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
											   auto fieldNode = nodeFactory.Field(field);
											   expr = nodeFactory.Expr(CNodeKind::EXPR_DOT, x, fieldNode);
										   }
										   //else if (type->GetKind() == TypeKind::Pointer)
										   //{
										   // // y 必定是整数，此时是用指针的高字节赋值 z = *((char*)&x + y)，因为指针占2个字节
										   // assert(tac->y.IsInterger());
										   // int offset = tac->y.GetValue();
										   // // (1) 取指针的地址 &x
										   // expr = nodeFactory.Expr(CNodeKind::EXPR_ADDR, x);
										   // // (2) 强制类型转换为 (char*)&x
										   // expr = nodeFactory.Expr(TypeManager::pValue, expr);
										   // // (3) 可选的偏移字节 (char*)&x + offset
										   // if (offset > 0)
										   // {
										   //  CNode* offsetNode = nodeFactory.Expr(offset);
										   //  expr = nodeFactory.Expr(CNodeKind::EXPR_ADD, expr, offsetNode);
										   // }
										   // // 解引用
										   // expr = nodeFactory.Expr(CNodeKind::EXPR_DEREF, expr);
										   //}
										   else
										   {
											   expr = nodeFactory.Expr(CNodeKind::EXPR_INDEX, x, GetExpression(tac->y));
										   }
										   current = GetNodeFactory().AssignStat(GetExpression(tac->z), expr);
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
											   auto fieldNode = nodeFactory.Field(field);
											   expr = nodeFactory.Expr(CNodeKind::EXPR_DOT, x, fieldNode);
										   }
										   //else if (type->GetKind() == TypeKind::Pointer)
										   //{
										   // COUT << tac;
										   // // y 必定是整数，此时是用指针的高字节赋值 z = *((char*)&x + y)，因为指针占2个字节
										   // assert(tac->y.IsInterger());
										   // int offset = tac->y.GetValue();
										   // // (1) 取指针的地址 &x
										   // expr = nodeFactory.Expr(CNodeKind::EXPR_ADDR, x);
										   // // (2) 强制类型转换为 (char*)&x
										   // expr = nodeFactory.Expr(TypeManager::pValue, expr);
										   // // (3) 可选的偏移字节 (char*)&x + offset
										   // if (offset > 0)
										   // {
										   //  CNode* offsetNode = nodeFactory.Expr(offset);
										   //  expr = nodeFactory.Expr(CNodeKind::EXPR_ADD, expr, offsetNode);
										   // }
										   // // 解引用
										   // expr = nodeFactory.Expr(CNodeKind::EXPR_DEREF, expr);
										   //}
										   else
										   {
											   expr = nodeFactory.Expr(CNodeKind::EXPR_INDEX, x, GetExpression(tac->y));
										   }
										   current = nodeFactory.AssignStat(expr, GetExpression(tac->z));
										   break;
			}
			case TACOperator::ADDR:
				current = UnaryExpression(CNodeKind::EXPR_ADDR, tac);
				break;
			case TACOperator::DEREF:
				current = UnaryExpression(CNodeKind::EXPR_DEREF, tac);
				break;
			case TACOperator::CAST:
			{
									  auto result = GetExpression(tac->z);
									  assert(result->kind == CNodeKind::EXPR_VARIABLE);
									  auto type = result->variable->type;  // 要转换到的类型
									  expr = nodeFactory.Cast(type, GetExpression(tac->x));
									  expr = nodeFactory.Expr(CNodeKind::EXPR_ASSIGN, result, expr);
									  current = nodeFactory.ExprStat(expr);
									  break;
			}
			case	TACOperator::ARG:
			{
				// 若干个 ARG 后面跟着一个 CALL
				// 遇到 ARG，则要连着后面的直到 CALL 的三地址码一起翻译
				CNode* argsNode = GetNodeFactory().ExprList();
				CListNode args(argsNode);

				while (codes[i]->op == TACOperator::ARG)
				{
					args.Add(GetExpression(codes[i]->x));
					++i;
				}
				if (codes[i]->op != TACOperator::CALL)
					throw Exception(_T("三地址码翻译为C语句：ARG 后面不是 CALL"));
				// 最后是 CALL 指令
				current = TranslateCall(codes[i], argsNode);
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
				current = nodeFactory.Goto(GetLabelName(tac->z.GetValue()));
				break;
			case TACOperator::BIT:
			{
									 // 先这样翻译凑合一下，翻译成表达式语句
									 expr = nodeFactory.Expr(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
									 current = nodeFactory.ExprStat(expr);
									 break;
			}
			case TACOperator::RETURN:
			{
										if (tac->x.IsZero())  // 目前只能返回 AXY 对象，所以可以这么判断有没有返回值
										{
											current = nodeFactory.Expr(CNodeKind::STAT_RETURN);
											break;
										}
										// 有返回值的情况
										current = nodeFactory.Expr(CNodeKind::STAT_RETURN, GetExpression(tac->x));
										break;
			}
			case TACOperator::ROR:
			{
									 // C语言中没有ROR运算符，翻译为函数调用好了
									 // void Ror(int*, int)
				CNode* listNode = nodeFactory.ExprList();
				CListNode list(listNode);
				CNode* params = nodeFactory.Expr(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
				list.Add(params);
				list.Add(GetExpression(tac->y));
				current = nodeFactory.Call(GetCDB().AddString(_T("Ror")), listNode);
									 break;
			}
			case TACOperator::ROL:
			{
									 // C语言中没有ROL运算符，翻译为函数调用好了
									 // void Rol(int*, int)
				CNode* listNode = nodeFactory.ExprList();
				CListNode list(listNode);
				CNode* params = nodeFactory.Expr(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
				list.Add(params);
				list.Add(GetExpression(tac->y));
				current = nodeFactory.Call(GetCDB().AddString(_T("Rol")), listNode);
									 break;
			}
			case TACOperator::PUSH:
			{
									  // 还不知道怎么翻译push，先翻译为函数调用吧
									  CNode* params = GetExpression(tac->x);
									  expr = nodeFactory.Call(GetCDB().AddString(_T("Push")), params);
									  current = nodeFactory.ExprStat(expr);
									  break;
			}
			case TACOperator::POP:
			{
									 // 还不知道怎么翻译pop，先翻译为函数调用吧
									 expr = nodeFactory.Call(GetCDB().AddString(_T("Pop")), (CNode*)nullptr);
									 current = nodeFactory.AssignStat(GetExpression(tac->z), expr);
									 break;
			}
			case TACOperator::BOOL_FLAGV:
			{
											// 翻译为函数调用
				CNode* listNode = nodeFactory.ExprList();
				CListNode list(listNode);
				list.Add(GetExpression(tac->x));
				list.Add(GetExpression(tac->y));
				expr = nodeFactory.Call(GetCDB().AddString(_T("IsOverflow")), (CNode*)nullptr);
				current = nodeFactory.AssignStat(GetExpression(tac->z), expr);
											break;
			}

			case TACOperator::CLI:
			{
									 // 翻译为函数调用
									 expr = nodeFactory.Call(GetCDB().AddString(_T("Cli")), (CNode*)nullptr);
									 current = nodeFactory.ExprStat(expr);
									 break;
			}
			case TACOperator::SEI:
			{
									 // 翻译为函数调用
									 expr = nodeFactory.Call(GetCDB().AddString(_T("Sei")), (CNode*)nullptr);
									 current = nodeFactory.ExprStat(expr);
									 break;
			}
			case TACOperator::CLD:
			{
									 // 翻译为函数调用
									 expr = nodeFactory.Call(GetCDB().AddString(_T("Cld")), (CNode*)nullptr);
									 current = nodeFactory.ExprStat(expr);
									 break;
			}
			case TACOperator::SED:
			{
									 // 翻译为函数调用
									 expr = nodeFactory.Call(GetCDB().AddString(_T("Sed")), (CNode*)nullptr);
									 current = nodeFactory.ExprStat(expr);
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
			list.Add(current);
			AddAddressMapStatement(tac->address, current);  // 记录每条语句对应的地址
		}
		auto blockStat = NewStatementList(list);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
		funcList.Add(blockStat);
	}
	auto funcHead = NewStatementList(funcList);
	// 在全部语句都生成后，回填标签语句
	PatchLabels();

	return funcHead;
}
