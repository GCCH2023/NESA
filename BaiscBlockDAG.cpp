#include "stdafx.h"
#include "BaiscBlockDAG.h"
#include "TAC.h"
#include "CDataBase.h"
#include "Variable.h"

std::size_t CNodeHash::operator()(const CNode* node) const
{
	size_t hash = (size_t)node->kind;
	switch (node->kind)
	{
	case CNodeKind::STAT_LIST:
		for (auto n = node->list.head; n; n = n->next)
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
	case CNodeKind::STAT_LABEL:
		hash ^= (size_t)node->l.name ^ (size_t)node->l.body;
		break;
	case CNodeKind::STAT_NONE:
		break;
	case CNodeKind::STAT_CALL:
		hash ^= (size_t)node->f.name;
		for (auto n = node->f.params; n; n = n->next)
			hash ^= (size_t)n;
		break;
	case CNodeKind::STAT_RETURN:
		hash ^= (size_t)node->e.x;
		break;
	case CNodeKind::EXPR_INTEGER:
		hash ^= node->i.value;
	case CNodeKind::EXPR_VARIABLE:
		hash ^= (size_t)node->variable->name;
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
	case CNodeKind::EXPR_REF:
	case CNodeKind::EXPR_ADDR:
		hash ^= (size_t)node->e.x;
		break;

	default:
		throw Exception(_T("计算抽象语法树节点哈希值: 未实现的节点类型"));
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
	case CNodeKind::STAT_CALL:
		if (node1->f.name != node2->f.name)
			return false;
		{
			auto p = node1->f.params;
			auto q = node2->f.params;
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
	case CNodeKind::EXPR_REF:
	case CNodeKind::EXPR_ADDR:
		return node1->e.x == node2->e.x;
	default:
		throw Exception(_T("比较抽象语法树节相等: 未实现的节点类型"));
	}
}


BaiscBlockDAG::BaiscBlockDAG(Allocator& allocator_, CDataBase& cdb_):
allocator(allocator_),
cdb(cdb_)
{
	//registers[Nes::NesRegisters::A] = CNode(cdb.AddString(_T("A")));
	//registers[Nes::NesRegisters::X] = CNode(cdb.AddString(_T("X")));
	//registers[Nes::NesRegisters::Y] = CNode(cdb.AddString(_T("Y")));
	//registers[Nes::NesRegisters::P] = CNode(cdb.AddString(_T("P")));
	//registers[Nes::NesRegisters::SP] = CNode(cdb.AddString(_T("SP")));

}

BaiscBlockDAG::~BaiscBlockDAG()
{
}

String* BaiscBlockDAG::NewString(const CStr format, ...)
{
	TCHAR buffer[256];
	va_list va;
	va_start(va, format);
	_vstprintf_s(buffer, 256, format, va);
	va_end(va);
	return cdb.AddString(buffer);
}


CNode* BaiscBlockDAG::GetExpression(TACOperand& operand)
{
	//// 首先查找最近的赋值语句
	//auto it = lastest.find(operand);
	//if (it != lastest.end())
	//	return it->second;

	//CNode node = { 0 };
	//switch (operand.GetKind())
	//{
	//case TACOperand::INTEGER:
	//	if (operand.IsTemp())
	//	{
	//		node.kind = CNodeKind::EXPR_VARIABLE;
	//		node.v.name = NewString(_T("temp%d"), operand.GetValue());
	//		node.v.varKind = VAR_KIND_LOCAL;
	//	}
	//	else
	//	{
	//		node.kind = CNodeKind::EXPR_INTEGER;
	//		node.i.value = operand.GetValue();
	//	}
	//	return GetNode(&node);
	//case TACOperand::REGISTER:
	//	return GetNode(&registers[operand.GetValue()]);
	//case TACOperand::MEMORY:
	//{
	//						   if (operand.IsTemp())
	//						   {
	//							   node.kind = CNodeKind::EXPR_VARIABLE;
	//							   node.v.name = NewString(_T("temp%d"), operand.GetValue());
	//							   node.v.varKind = VAR_KIND_GLOBAL;
	//							   auto v = GetNode(&node);
	//							   // 需要解引用
	//							   node = CNode(CNodeKind::EXPR_REF, v);
	//							   return GetNode(&node);
	//						   }
	//						   node.kind = CNodeKind::EXPR_VARIABLE;
	//						   node.v.name = NewString(_T("g_%04X"), operand.GetValue());
	//						   node.v.varKind = VAR_KIND_GLOBAL;
	//						   return GetNode(&node);
	//}
	//case TACOperand::ADDRESS:
	//{
	//							node.kind = CNodeKind::EXPR_VARIABLE;
	//							node.v.name = NewString(_T("g_%04X"), operand.GetValue());
	//							node.v.varKind = VAR_KIND_GLOBAL;
	//							return GetNode(&node);
	//}
	//default:
	//{
	//		   TCHAR buffer[64];
	//		   _stprintf_s(buffer, _T("三地址码转C语句：未实现的三地址码操作数转换"));
	//		   throw Exception(buffer);
	//}
	//}
	return nullptr;
}


CNode* BaiscBlockDAG::GetNode(CNode* node)
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

CNode* BaiscBlockDAG::ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr)
{
	// 条件跳转指令必定是基本块结束指令
	condition = allocator.New<CNode>(kind, GetExpression(tac->x), GetExpression(tac->y));
	jumpAddr = tac->z.GetValue();
	return condition;
}

CNode* BaiscBlockDAG::NewNoneStatement()
{
	return allocator.New<CNode>(CNodeKind::STAT_NONE);
}


CNode* BaiscBlockDAG::NewStatementList(CNode* head, CNode* tail)
{
	if (!head)
		return NewNoneStatement();
	if (head == tail)
		return head;
	return allocator.New<CNode>(CNodeKind::STAT_LIST, head, tail);
}


CNode* BaiscBlockDAG::Translate(TACBasicBlock* tacBlock)
{
	CNode* current = nullptr, *head = nullptr, *tail = nullptr;
	CNode* expr = nullptr;
	auto& codes = tacBlock->GetCodes();
	CNode* x, *y;
	CNode temp, *node;
	CNode* pCondition;
	uint32_t jumpAddr;
	for (size_t i = 0; i < codes.size(); ++i)
	{
		auto tac = codes[i];
		switch (tac->op)
		{
		case	TACOperator::BOR:
			x = GetExpression(tac->x);
			y = GetExpression(tac->y);
			temp = CNode(CNodeKind::EXPR_BOR, x, y);
			node = GetNode(&temp);

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
			x = GetExpression(tac->x);
			y = GetExpression(tac->y);
			temp = CNode(CNodeKind::EXPR_ADD, x, y);
			node = GetNode(&temp);
			lastest[tac->z] = node;
			break;
		case	TACOperator::SUB:
			x = GetExpression(tac->x);
			y = GetExpression(tac->y);
			temp = CNode(CNodeKind::EXPR_SUB, x, y);
			node = GetNode(&temp);
			lastest[tac->z] = node;
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
									auto call = allocator.New<CNode>(NewString(_T("sub_%04X"), codes[i]->x.GetValue()), params);
									current = call;
									break;
		}
		case	TACOperator::CALL:
		{
									 // 如果有参数，则鄙视 若干个 ARG 后面跟着一个 CALL
									 // 直接出现 CALL，说明没有参数
									 current = allocator.New<CNode>(NewString(_T("sub_%04X"), tac->x.GetValue()), (CNode*)nullptr);
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
									// 返回值以后再考虑吧
									current = allocator.New<CNode>(CNodeKind::STAT_RETURN);
									break;
		}
		case TACOperator::ROR:
		{
								 // C语言中没有ROR运算符，翻译为函数调用好了
								 // void Ror(int*, int)
								 CNode* params = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
								 params->next = GetExpression(tac->y);
								 current = allocator.New<CNode>(NewString(_T("Ror")), params);
								 break;
		}
		case TACOperator::ROL:
		{
								 // C语言中没有ROL运算符，翻译为函数调用好了
								 // void Rol(int*, int)
								 CNode* params = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
								 params->next = GetExpression(tac->y);
								 current = allocator.New<CNode>(NewString(_T("Rol")), params);
								 break;
		}
		case TACOperator::PUSH:
		{
								  // 还不知道怎么翻译push，先翻译为函数调用吧
								  CNode* params = GetExpression(tac->x);
								  current = allocator.New<CNode>(NewString(_T("Push")), params);
								  break;
		}
		case TACOperator::POP:
		{
								 // 还不知道怎么翻译pop，先翻译为函数调用吧
								 current = allocator.New<CNode>(NewString(_T("Pop")), (CNode*)nullptr);
								 current = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), current);
								 break;
		}

		case TACOperator::CLI:
		{
								 // 翻译为函数调用
								 current = allocator.New<CNode>(NewString(_T("Cli")), (CNode*)nullptr);
								 break;
		}
		case TACOperator::SEI:
		{
								 // 翻译为函数调用
								 current = allocator.New<CNode>(NewString(_T("Sei")), (CNode*)nullptr);
								 break;
		}
		case TACOperator::CLD:
		{
								 // 翻译为函数调用
								 current = allocator.New<CNode>(NewString(_T("Cld")), (CNode*)nullptr);
								 break;
		}
		case TACOperator::SED:
		{
								 // 翻译为函数调用
								 current = allocator.New<CNode>(NewString(_T("Sed")), (CNode*)nullptr);
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
	//Nes::Address firstAddr = codes.empty() ? tacBlock->GetStartAddress() : codes[0]->address;
	//auto ret = NewStatementList(head, tail);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
	// blockStatements[firstAddr] = ret;  // 记录下这个基本块对应的地址及语句

	CNode* ret = nullptr;
	// 遍历变量的赋值点
	for (auto& it : lastest)
	{
		// 未实现
	}
	return ret;
}

std::size_t TACOperandHash::operator()(const TACOperand& operand) const
{
	return operand.GetHash();
}
