#include "stdafx.h"
#include "CBasicBlockDAGTranslator.h"
#include "TACFunction.h"
#include "CNode.h"
#include "CDataBase.h"
#include "CTranslator.h"

std::size_t CNodeHash::operator()(const CNode* node) const
{
	size_t hash = (size_t)node->kind;
	switch (node->kind)
	{
	case CNodeKind::STAT_LIST:
		for (auto n = node->list.head; n; n = n->GetNext())
			hash ^= (size_t)n;
		break;
	case CNodeKind::STAT_EXPR:
		hash ^= (size_t)node->e.x;
		break;
	case CNodeKind::STAT_WHILE:
	case CNodeKind::STAT_DO_WHILE:
		hash ^= (size_t)node->s.then ^ (size_t)node->s.condition;
		break;
	case CNodeKind::STAT_IF:
		hash ^= (size_t)node->s.then ^ (size_t)node->s.condition ^ (size_t)node->s._else;
		break;
	case CNodeKind::STAT_GOTO:
		hash ^= (size_t)node->l.name;
		break;
	case CNodeKind::STAT_LABEL:
		hash ^= (size_t)node->l.name ^ (size_t)node->l.body;
		break;
	case CNodeKind::STAT_NONE:
		break;
	case CNodeKind::EXPR_CALL:
		hash ^= (size_t)node->call.name;
		for (auto n = node->call.params; n; n = n->GetNext())
			hash ^= (size_t)n;
		break;
	case CNodeKind::STAT_RETURN:
		hash ^= (size_t)node->e.x;
		break;
	case CNodeKind::EXPR_INTEGER:
		hash ^= node->i.value;
		break;
	case CNodeKind::EXPR_VARIABLE:
		hash ^= (size_t)node->variable->name;
		break;
	case CNodeKind::EXPR_BOR:
	case CNodeKind::EXPR_BAND:
	case CNodeKind::EXPR_XOR:
	case CNodeKind::EXPR_SHIFT_LEFT:
	case CNodeKind::EXPR_SHIFT_RIGHT:
	case CNodeKind::EXPR_ADD:
	case CNodeKind::EXPR_SUB:
	case CNodeKind::EXPR_AND:
	case CNodeKind::EXPR_OR:
	case CNodeKind::EXPR_ASSIGN:
	case CNodeKind::EXPR_GREAT:
	case CNodeKind::EXPR_GREAT_EQUAL:
	case CNodeKind::EXPR_EQUAL:
	case CNodeKind::EXPR_NOT_EQUAL:
	case CNodeKind::EXPR_LESS:
	case CNodeKind::EXPR_LESS_EQUAL:
	case CNodeKind::EXPR_INDEX:
		hash ^= (size_t)node->e.x ^ (size_t)node->e.y;
		break;

	case CNodeKind::EXPR_NOT:
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
		hash ^= (size_t)node->e.x;
		break;

	case CNodeKind::EXPR_CAST:
		hash ^= (size_t)node->cast.type ^ (size_t)node->cast.expr;
		break;

	default:
	{
		Sprintf<> s;
		s.Format(_T("计算抽象语法树节点哈希值: 未实现的节点类型 %s"), ToString(node->kind));
		throw Exception(s.ToString());
		break;
	}
	}
	return hash;
}

bool CNodeEqual::operator()(const CNode* node1, const CNode* node2) const
{
	if (node1->kind != node2->kind)
		return false;
	switch (node1->kind)
	{
	case CNodeKind::STAT_LIST:
	{
		auto p = node1->list.head;
		auto q = node2->list.head;
		while (p++ == q++)
			;
		return p == q;
	}
	case CNodeKind::STAT_EXPR:
		return node1->e.x == node2->e.x;
	case CNodeKind::STAT_WHILE:
	case CNodeKind::STAT_DO_WHILE:
		return node1->s.condition == node2->s.condition && node1->s.then == node2->s.then;
	case CNodeKind::STAT_IF:
		return node1->s.condition == node2->s.condition &&
			node1->s.then == node2->s.then &&
			node1->s._else == node2->s._else;
	case CNodeKind::STAT_GOTO:
		return node1->l.name == node2->l.name;
	case CNodeKind::STAT_LABEL:
		return node1->l.name == node2->l.name && node1->l.body == node2->l.body;
	case CNodeKind::STAT_NONE:
		return true;
	case CNodeKind::EXPR_CALL:
		if (node1->call.name != node2->call.name)
			return false;
		{
			auto p = node1->call.params;
			auto q = node2->call.params;
			while (p++ == q++)
				;
			return p == q;
		}
	case CNodeKind::EXPR_INTEGER:
		return node1->i.value == node2->i.value;
	case CNodeKind::EXPR_VARIABLE:
		return node1->variable->name == node2->variable->name;
	case CNodeKind::EXPR_BOR:
	case CNodeKind::EXPR_BAND:
	case CNodeKind::EXPR_XOR:
	case CNodeKind::EXPR_SHIFT_LEFT:
	case CNodeKind::EXPR_SHIFT_RIGHT:
	case CNodeKind::EXPR_ADD:
	case CNodeKind::EXPR_SUB:
	case CNodeKind::EXPR_AND:
	case CNodeKind::EXPR_OR:
	case CNodeKind::EXPR_ASSIGN:
	case CNodeKind::EXPR_GREAT:
	case CNodeKind::EXPR_GREAT_EQUAL:
	case CNodeKind::EXPR_EQUAL:
	case CNodeKind::EXPR_NOT_EQUAL:
	case CNodeKind::EXPR_LESS:
	case CNodeKind::EXPR_LESS_EQUAL:
	case CNodeKind::EXPR_INDEX:
		return node1->e.x == node2->e.x && node1->e.y == node2->e.y;

	case CNodeKind::STAT_RETURN:
	case CNodeKind::EXPR_NOT:
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
		return node1->e.x == node2->e.x;

	case CNodeKind::EXPR_CAST:
		return node1->cast.type == node2->cast.type && node1->cast.expr == node2->cast.expr;

	default:
	{
		Sprintf<> s;
		s.Format(_T("比较抽象语法树节点相等: 未实现的节点类型 %s"), ToString(node1->kind));
		throw Exception(s.ToString());
		break;
	}
	}
}

CBasicBlockDAGTranslator::CBasicBlockDAGTranslator(CTranslator* translator_) :
	translator(translator_),
	allocator(translator_->GetAllocator()),
	condition(nullptr),
	jumpAddr(0)
{
}

CNode* CBasicBlockDAGTranslator::GetExpression(const TACOperand& operand)
{
	auto it = varMap.find(operand);
	if (it != varMap.end())
		return it->second;

	CNode node;
	translator->GetExpression(node, operand);
	return GetNode(&node);
}

const Variable* CBasicBlockDAGTranslator::GetVariable(const TACOperand& operand)
{
	CNode node;
	translator->GetExpression(node, operand);
	assert(node.kind == CNodeKind::EXPR_VARIABLE);
	return node.variable;
}

CNode* CBasicBlockDAGTranslator::TranslateCall(TAC* call, CNode* params)
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
		name = translator->GetLocalVariableName(call->x.GetValue());
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

CNode* CBasicBlockDAGTranslator::ConditionalJump(CNodeKind kind, TAC* tac, uint32_t& jumpAddr)
{
	// 条件跳转指令必定是基本块结束指令
	condition = allocator.New<CNode>(kind, GetExpression(tac->x), GetExpression(tac->y));
	jumpAddr = tac->z.GetValue();
	return condition;
}

CNode* CBasicBlockDAGTranslator::GetNode(CNode* node)
{
	auto it = nodeSet.find(node);
	if (it != nodeSet.end())
		return *it;
	// 创建
	CNode* n = allocator.New<CNode>();
	*n = *node;
	nodeSet.insert(n);
	return n;
}

void CBasicBlockDAGTranslator::Attach(TACOperand& var, CNode* node)
{
	varMap[var] = node;
}

void CBasicBlockDAGTranslator::GenerateDAG(TACBasicBlock* block)
{
	CNode* current = nullptr, * head = nullptr, * tail = nullptr;
	CNode* expr = nullptr;
	CNode* x;

	auto& codes = block->GetCodes();
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
			Attach(tac->z, GetExpression(tac->x));
			break;
		case	TACOperator::ADD:
			expr = allocator.New<CNode>(CNodeKind::EXPR_ADD, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::SUB:
			x = GenNode(CNodeKind::EXPR_SUB, GetExpression(tac->x), GetExpression(tac->y));
			Attach(tac->z, x);
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
			x = GenNode(CNodeKind::EXPR_GREAT_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
			Attach(tac->z, x);
			break;
		case TACOperator::BOOL_LESS:
			x = GenNode(CNodeKind::EXPR_LESS, GetExpression(tac->x), GetExpression(tac->y));
			Attach(tac->z, x);
			break;
			break;
		case TACOperator::BOOL_LEQ:
			expr = allocator.New<CNode>(CNodeKind::EXPR_LESS_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::BOOL_EQ:
			x = GenNode(CNodeKind::EXPR_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
			Attach(tac->z, x);
			break;
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
			auto variable = GetVariable(tac->x);
			auto type = variable->type;
			if (type->GetKind() == TypeKind::Struct)
			{
				// y 必定是整数
				assert(tac->y.IsInterger());
				// 根据偏移量查找字段
				auto field = type->GetField(tac->y.GetValue());
				auto fieldNode = allocator.New<CNode>(field);
				expr = allocator.New<CNode>(CNodeKind::EXPR_DOT, GetExpression(tac->x), fieldNode);
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
				expr = allocator.New<CNode>(CNodeKind::EXPR_INDEX, GetExpression(tac->x), GetExpression(tac->y));
			}
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;

		}
		case TACOperator::ARRAY_SET:
		{
			// 可能是给结构体字段赋值
			auto variable = GetVariable(tac->x);
			auto type = variable->type;
			if (type->GetKind() == TypeKind::Struct)
			{
				// y 必定是整数
				assert(tac->y.IsInterger());
				// 根据偏移量查找字段
				auto field = type->GetField(tac->y.GetValue());
				auto fieldNode = GenNode(field);
				expr = GenNode(CNodeKind::EXPR_DOT, GetExpression(tac->x), fieldNode);
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
				expr = GenNode(CNodeKind::EXPR_INDEX, GetExpression(tac->x), GetExpression(tac->y));
			}
			GenNode(CNodeKind::EXPR_ASSIGN, expr, GetExpression(tac->z));
			break;
		}
		case TACOperator::ADDR:
			x = GenNode(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
			Attach(tac->z, x);
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

			x = GenNode(type, GetExpression(tac->x));
			Attach(tac->z, x);
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
					paramsTail->SetNext(GetExpression(codes[i]->x));
					paramsTail = paramsTail->GetNext();
				}
				++i;
			}
			if (codes[i]->op != TACOperator::CALL)
				throw Exception(_T("三地址码翻译为C语句：ARG 后面不是 CALL"));
			// 最后是 CALL 指令
			current = TranslateCall(codes[i], params);
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
			continue;
		case TACOperator::IFGREAT:
			ConditionalJump(CNodeKind::EXPR_GREAT, tac, jumpAddr);
			continue;
		case TACOperator::IFEQ:
			ConditionalJump(CNodeKind::EXPR_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFNEQ:
			ConditionalJump(CNodeKind::EXPR_NOT_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFLESS:
			ConditionalJump(CNodeKind::EXPR_LESS, tac, jumpAddr);
			continue;
		case TACOperator::IFLEQ:
			ConditionalJump(CNodeKind::EXPR_LESS_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFTRUE:
			condition = allocator.New<CNode>(CNodeKind::EXPR_NOT_EQUAL,
				GetExpression(tac->x), GetExpression(TACOperand(0)));
			jumpAddr = tac->z.GetValue();
			continue;
		case TACOperator::IFFALSE:
			condition = allocator.New<CNode>(CNodeKind::EXPR_EQUAL, GetExpression(tac->x), GetExpression(TACOperand(0)));
			jumpAddr = tac->z.GetValue();
			continue;
		case TACOperator::GOTO:
		{
			// 新：当作条件总是真的跳转语句来翻译
			condition = allocator.New<CNode>(1);
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
			// condition = allocator.New<CNode>(1);
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
			params->SetNext(GetExpression(tac->y));
			current = allocator.New<CNode>(GetCDB().AddString(_T("Ror")), params);
			break;
		}
		case TACOperator::ROL:
		{
			// C语言中没有ROL运算符，翻译为函数调用好了
			// void Rol(int*, int)
			CNode* params = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
			params->SetNext(GetExpression(tac->y));
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
			params->SetNext(GetExpression(tac->y));
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
			tail->SetNext(current);
			tail = current;
		}
	}
	Nes::Address firstAddr = codes.empty() ? block->GetStartAddress() : codes[0]->address;
	auto ret = translator->NewStatementList(head, tail);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
	translator->AddAddressMapStatement(firstAddr, ret);  // 记录下这个基本块对应的地址及语句
}

TACBasicBlock* curBlock;
#include "Dump.h"

CNode* CBasicBlockDAGTranslator::GenerateCodes()
{
	// 遍历变量，生成它们的赋值语句
	CNode* head = nullptr, * tail = nullptr, *current = nullptr;
	for (auto it : varMap)
	{
		if (it.first.IsTemp())
			continue;
		// 生成赋值语句
		CNode* right = GenerateExpression(it.second);
		CNode* left = allocator.New<CNode>(GetVariable(it.first));
		current = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, left, right);
		if (curBlock->GetStartAddress() == 0x90CC)
		{
			COUT << current << std::endl;
		}
		// 构建语句列表
		if (!head)
		{
			head = tail = current;
		}
		else
		{
			tail->SetNext(current);
			tail = current;
		}
	}
	return translator->NewStatementList(head, tail);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
}

CNode* CBasicBlockDAGTranslator::GenerateExpression(CNode* node)
{
	switch (node->kind)
	{
	//case CNodeKind::EXPR_CALL:
	//{
	//	CNode* params;
	//	hash ^= (size_t)node->call.name;
	//	for (auto n = node->call.params; n; n = n->GetNext())
	//	{
	//		auto param = GenerateExpression(n);
	//		if (params == nullptr)
	//	}
	//	break;
	//}
	case CNodeKind::EXPR_INTEGER:
		return allocator.New<CNode>(node->i.value);
	case CNodeKind::EXPR_VARIABLE:
		return allocator.New<CNode>(node->variable);
	case CNodeKind::EXPR_BOR:
	case CNodeKind::EXPR_BAND:
	case CNodeKind::EXPR_XOR:
	case CNodeKind::EXPR_SHIFT_LEFT:
	case CNodeKind::EXPR_SHIFT_RIGHT:
	case CNodeKind::EXPR_ADD:
	case CNodeKind::EXPR_SUB:
	case CNodeKind::EXPR_AND:
	case CNodeKind::EXPR_OR:
	case CNodeKind::EXPR_ASSIGN:
	case CNodeKind::EXPR_GREAT:
	case CNodeKind::EXPR_GREAT_EQUAL:
	case CNodeKind::EXPR_EQUAL:
	case CNodeKind::EXPR_NOT_EQUAL:
	case CNodeKind::EXPR_LESS:
	case CNodeKind::EXPR_LESS_EQUAL:
	case CNodeKind::EXPR_INDEX:
		return allocator.New<CNode>(node->kind, node->e.x, node->e.y);

	case CNodeKind::EXPR_NOT:
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
		return allocator.New<CNode>(node->kind, node->e.x);

	default:
	{
		Sprintf<> s;
		s.Format(_T("DAG 节点转 AST 节点: 未实现的节点类型 % s"), ToString(node->kind));
		throw Exception(s.ToString());
		break;
	}
	}
}

// 临时变量必定是两条三地址码连着，所以直接合并成一个表达式
CNode* CBasicBlockDAGTranslator::Translate(TACBasicBlock* block)
{
	GenerateDAG(block);
	curBlock = block;
	auto node = GenerateCodes();
	if (block->GetStartAddress() == 0x90CC)
	{
		Sprintf<> s;
		COUT << s.Format(_T("\n block %04X - %04X\n"), block->GetStartAddress(), block->GetEndAddress());
		COUT << node;
		int a = 0;
	}
	return node;
}
