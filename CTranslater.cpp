#include "stdafx.h"
#include "CTranslater.h"
using namespace std;
#include "Function.h"
#include "CDataBase.h"

CTranslater::CTranslater(Allocator& allocator_, NesDataBase& db_) :
db(db_),
allocator(allocator_),
function(nullptr),
subroutine(nullptr),
tempAllocator(1024 * 1024)
{
	registers[Nes::NesRegisters::A] = NewString(_T("A"));
	registers[Nes::NesRegisters::X] = NewString(_T("X"));
	registers[Nes::NesRegisters::Y] = NewString(_T("Y"));
	registers[Nes::NesRegisters::P] = NewString(_T("P"));
	registers[Nes::NesRegisters::SP] = NewString(_T("SP"));
}


CTranslater::~CTranslater()
{

}

Function* CTranslater::TranslateSubroutine(TACFunction* subroutine)
{
	if (!subroutine)
		return nullptr;
	Reset();

	this->subroutine = subroutine;

	// 首先构造边集
	vector<Edge> edges(32);
	edges.clear();
	auto& blocks = subroutine->GetBasicBlocks();
	// 给基本块编号
	for (size_t i = 0; i < blocks.size(); ++i)
	{
		blocks[i]->tag = (void*)i;
	}
	for (auto block : blocks)
	{
		for (auto succ : block->nexts)
		{
			edges.push_back({ (int)block->tag, (int)succ->tag });
		}
	}

	Function* func = allocator.New<Function>();
	this->function = func;
	// 设置C函数的类型和参数
	SetFunctionType();
	// 创建临时变量
	SetLocalVariables();

	auto root = Analyze(edges.data(), edges.size());

	// 遍历控制树，生成C语句
	// COUT << root->statement;
	func->SetBody(root->statement);
	TCHAR buffer[64];
	_stprintf_s(buffer, _T("sub_%04X"), subroutine->GetStartAddress());
	func->name = GetCDB().AddString(buffer);


	return func;
}

ControlTreeNodeEx* CTranslater::Analyze(Edge edges[], size_t count)
{
	BuildCFG(edges, count);
	NodeSet N = CAnalysis(GetFullSet());
	if (N.GetSize() != 1)  // 也可能只有一个基本块
	{
		DumpCurrentCFG(N);
		throw Exception(_T("控制树无法归约到单一根节点"));
	}
	// 在全部语句都生成后，回填标签语句
	PatchLabels();

	Node n = N.ToVector()[0];
	if (ctrees[n]->statement == nullptr)
	{
		CNode* condition = nullptr;
		uint32_t jumpAddr;
		auto block = this->subroutine->GetBasicBlocks()[n];
		ctrees[n]->statement = TranslateRegion(condition, block, jumpAddr);
	}
	return ctrees[n];
}

void CTranslater::BuildCFG(Edge edges[], size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		Edge& p = edges[i];
		int first = p.from;
		int second = p.to;
		blocks[first].succ |= 1 << second;
		blocks[second].pred |= 1 << first;

		if (blockCount < first)
			blockCount = first;
		if (blockCount < second)
			blockCount = second;
	}
	++blockCount;
	for (int i = 0; i < blockCount; ++i)
	{
		blocks[i].index = i;
	}
}

void CTranslater::Reset()
{
	memset(blocks, 0, sizeof(blocks));
	memset(ctrees, 0, sizeof(ctrees));
	blockCount = 0;
	controlTreeNodeCount = 0;
	tempAllocator.Reset();
	function = nullptr;
	subroutine = nullptr;
	labels.clear();
	blockStatements.clear();
}

CNode* CTranslater::GetExpression(TACOperand& operand)
{
	switch (operand.GetKind())
	{
	case TACOperand::INTEGER:
		if (operand.IsTemp())
		{
			auto var = GetLocalVariable(operand.GetValue());
			return allocator.New<CNode>(var);
		}
		else
			return allocator.New<CNode>(operand.GetValue());
	case TACOperand::REGISTER:
	{
								 // 寄存器要么是参数，要么是局部变量，不能当作全局变量处理
								 auto name = registers[operand.GetValue()];
								 auto variable = this->function->GetParameter(name);
								 if (variable)
									 return allocator.New<CNode>(variable);
								 variable = GetLocalVariable(name, TypeManager::Char);
								 return allocator.New<CNode>(variable);
	}
	case TACOperand::MEMORY:
	{
							   if (operand.IsTemp())
							   {
								   auto var = GetLocalVariable(operand.GetValue());
								   auto varNode = allocator.New<CNode>(var);
								   // 需要解引用
								   return allocator.New<CNode>(CNodeKind::EXPR_DEREF, varNode);
							   }
							   uint32_t addr = operand.GetValue();
							   auto global = GetCDB().GetGlobalVariable(addr);
							   if (!global)
							   {
								   Sprintf<> s;
								   s.Format(_T("获取全局变量 %X 失败"), addr);
								   throw Exception(s.ToString());
							   }
							   return allocator.New<CNode>(global);
	}
	case TACOperand::ADDRESS:
	{
								uint32_t addr = operand.GetValue();
								auto global = GetCDB().GetGlobalVariable(addr);
								if (!global)
								{
									Sprintf<> s;
									s.Format(_T("获取全局变量 %X 失败"), addr);
									throw Exception(s.ToString());
								}
								return allocator.New<CNode>(global);
	}
	default:
	{
			   TCHAR buffer[64];
			   _stprintf_s(buffer, _T("三地址码转C语句：未实现的三地址码操作数转换"));
			   throw Exception(buffer);
	}
	}
}


CNode* CTranslater::ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr)
{
	// 条件跳转指令必定是基本块结束指令
	condition = allocator.New<CNode>(kind, GetExpression(tac->x), GetExpression(tac->y));
	jumpAddr = tac->z.GetValue();
	return condition;
}


CNodeKind CTranslater::TranslateOperator(TACOperator op)
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

// 临时变量必定是两条三地址码连着，所以直接合并成一个表达式
CNode* CTranslater::TranslateRegion(CNode*& pCondition, TACBasicBlock* tacBlock, uint32_t& jumpAddr)
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
									   else
									   {
										   expr = allocator.New<CNode>(CNodeKind::EXPR_INDEX, x, GetExpression(tac->y));
									   }
									   expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, expr, GetExpression(tac->z));
									   current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
									   break;
		}
		case TACOperator::DEREF:
			expr = allocator.New<CNode>(CNodeKind::EXPR_DEREF, GetExpression(tac->x));
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
									tac = codes[i];
									uint32_t callAddr = tac->x.GetValue();
									expr = allocator.New<CNode>(NewString(_T("sub_%04X"), callAddr), params);
									// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
									if (tac->z.IsTemp())
									{
										expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
									}
									current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
									break;
		}
		case	TACOperator::CALL:
		{
									 // 如果有参数，则必是 若干个 ARG 后面跟着一个 CALL
									 // 直接出现 CALL，说明没有参数
									 uint32_t callAddr = codes[i]->x.GetValue();
									 expr = allocator.New<CNode>(NewString(_T("sub_%04X"), callAddr), (CNode*)nullptr);
									 // 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
									 if (tac->z.IsTemp())
									 {
										 expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
									 }
									 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
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
								 expr = allocator.New<CNode>(NewString(_T("Cli")), (CNode*)nullptr);
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
		case TACOperator::SEI:
		{
								 // 翻译为函数调用
								 expr = allocator.New<CNode>(NewString(_T("Sei")), (CNode*)nullptr);
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
		case TACOperator::CLD:
		{
								 // 翻译为函数调用
								 expr = allocator.New<CNode>(NewString(_T("Cld")), (CNode*)nullptr);
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
		case TACOperator::SED:
		{
								 // 翻译为函数调用
								 expr = allocator.New<CNode>(NewString(_T("Sed")), (CNode*)nullptr);
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
	blockStatements[firstAddr] = ret;  // 记录下这个基本块对应的地址及语句
	return ret;
}

//CLabelStatement* CTranslater::GetLabel(uint32_t jumpAddr)
//{
//	auto it = labels.find(jumpAddr);
//	if (it == labels.end())
//	{
//		auto name = GetLabelName(jumpAddr);
//		CLabelStatement* label = allocator.New<CLabelStatement>(name.c_str());  // 目标语句待定
//		labels[jumpAddr] = label;
//		return label;
//	}
//	return it->second;
//}


String* CTranslater::GetLabelName(uint32_t jumpAddr)
{
	auto it = labels.find(jumpAddr);
	if (it == labels.end())
	{
		auto label = NewString(_T("L%04X"), jumpAddr);
		labels[jumpAddr] = label;
		return label;
	}
	return it->second;
}


CNode* CTranslater::CombineListIf(CNode* statement, CNode* condition, CNode* body, CNode* elseBody /*= nullptr*/)
{
	auto ifStat = allocator.New<CNode>(CNodeKind::STAT_IF, condition, body, elseBody);
	if (!statement)
		return ifStat;
	return NewStatementPair(statement, ifStat);
}

CNode* CTranslater::GetNotExpression(CNode* expr)
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

void CTranslater::PatchLabels()
{
	for (auto label : labels)
	{
		auto statement = blockStatements[label.first];
		if (statement->kind != CNodeKind::STAT_LABEL)
		{
			// 修改为标签语句
			auto body = allocator.New<CNode>();
			*body = *statement;
			statement->kind = CNodeKind::STAT_LABEL;
			statement->l.body = body;
			statement->l.name = label.second;
		}
	}
}

String* CTranslater::NewString(const CStr format, ...)
{
	TCHAR buffer[256];
	va_list va;
	va_start(va, format);
	_vstprintf_s(buffer, 256, format, va);
	va_end(va);
	return GetCDB().AddString(buffer);
}

CNode* CTranslater::NewDoWhile(CNode* condition, CNode* body)
{
	// do ; while (condition) => while (condition) ;
	// 没有循环体或者条件总是为真，则转换为 while 循环
	if (body->kind == CNodeKind::STAT_NONE || condition->kind == CNodeKind::EXPR_INTEGER)
		return allocator.New<CNode>(CNodeKind::STAT_WHILE, condition, body);
	return allocator.New<CNode>(CNodeKind::STAT_DO_WHILE, condition, body);
}

CNode* CTranslater::NewStatementList(CNode* head, CNode* tail)
{
	if (!head)
		return NewNoneStatement();
	if (head == tail)
		return head;
	return allocator.New<CNode>(CNodeKind::STAT_LIST, head, tail);
}

CNode* CTranslater::NewStatementPair(CNode* first, CNode* second)
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

CNode* CTranslater::NewNoneStatement()
{
	return allocator.New<CNode>(CNodeKind::STAT_NONE);
}

void CTranslater::SetFunctionType()
{
	// 首先创建一个表示AXY寄存器的结构体
	Type* axyType = GetCDB().GetAXYType();

	// 创建函数类型
	Type funcType(TypeKind::Function);
	funcType.f.returnType = TypeManager::Void;
	if (this->subroutine->GetReturnFlag())
	{
		// 有返回值，那么就使用 AXY 结构体作为返回值
		funcType.f.returnType = axyType;
	}
	auto param = this->subroutine->GetParamFlag();
	Variable a;
	a.name = registers[Nes::NesRegisters::A];
	a.type = TypeManager::Char;
	TypeList aType = { TypeManager::Char, nullptr };
	Variable x;
	x.name = registers[Nes::NesRegisters::X];
	x.type = TypeManager::Char;
	TypeList xType = { TypeManager::Char, nullptr };
	Variable y;
	y.name = registers[Nes::NesRegisters::Y];
	y.type = TypeManager::Char;
	TypeList yType = { TypeManager::Char, nullptr };
	if (param)
	{
		if (param & (1 << Nes::NesRegisters::A))
		{
			funcType.AddParameter(&aType);
			this->function->AddParameter(allocator.New<Variable>(&a));
		}
		if (param & (1 << Nes::NesRegisters::X))
		{
			funcType.AddParameter(&xType);
			this->function->AddParameter(allocator.New<Variable>(&x));
		}
		if (param & (1 << Nes::NesRegisters::Y))
		{
			funcType.AddParameter(&yType);
			this->function->AddParameter(allocator.New<Variable>(&y));
		}
	}
	this->function->SetType(GetTypeManager().NewFunction(&funcType));
}


void CTranslater::SetLocalVariables()
{
	auto& types = this->subroutine->GetTempVariableTypes();
	int i = 0;
	for (auto type : types)
	{
		// 添加
		auto variable = allocator.New<Variable>();
		if (type == GetCDB().GetAXYType())
			variable->name = NewString(_T("axy"), i);
		else
			variable->name = NewString(_T("temp%d"), i);
		variable->type = type;
		this->function->AddVariable(variable);
		++i;
	}
}

// 当要将控制流图中的一个自循环节点归约时
// a -> a
void CTranslater::OnReduceSelfLoop(Node n)
{
	auto node = ctrees[n];
	if (node->type != CTNTYPE_LEAF)
	{
		if (node->condition == nullptr)
			throw Exception(_T("非叶子自循环节点异常"));
		node->statement = NewDoWhile(node->condition, node->statement);
		return;
	}
	CNode* condition = nullptr;
	auto block = this->subroutine->GetBasicBlocks()[node->index];
	uint32_t jumpAddr;
	auto a = TranslateRegion(condition, block, jumpAddr);
	node->statement = NewDoWhile(condition, a);
}

void CTranslater::OnReduceList(Node f, Node s)
{
	auto first = ctrees[f];
	auto second = ctrees[s];
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (first->type == CTNTYPE_LEAF)
	{
		first->statement = TranslateRegion(condition, blocks[first->index], jumpAddr);
	}
	if (second->type == CTNTYPE_LEAF)
	{
		second->statement = TranslateRegion(condition, blocks[second->index], jumpAddr);
	}
	first->statement = NewStatementPair(first->statement, second->statement);
	first->condition = condition;
}

void CTranslater::OnReducePoint2Loop(Node f, Node s)
{
	auto first = ctrees[f];
	auto second = ctrees[s];
	auto node = first;  // 结果
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (first->type == CTNTYPE_LEAF)
	{
		first->statement = TranslateRegion(condition, blocks[first->index], jumpAddr);
		// 跳转边翻译为 goto 语句
		auto gotoStat = allocator.New<CNode>(CNodeKind::STAT_GOTO, GetLabelName(jumpAddr));
		first->statement = CombineListIf(first->statement, condition, gotoStat);
	}
	else
	{
		throw Exception(_T("2点循环的第一个节点不是叶子节点的归约未实现"));
	}
	if (second->type == CTNTYPE_LEAF)
	{
		second->statement = TranslateRegion(condition, blocks[second->index], jumpAddr);
	}
	else
	{
		assert(second->statement);
		assert(second->condition);
		condition = second->condition;
	}
	node->statement = NewStatementPair(first->statement, second->statement);
	node->statement = NewDoWhile(condition, node->statement);
}

void CTranslater::OnReduceIf(Node _if, Node then)
{
	auto cond = ctrees[_if];
	auto body = ctrees[then];
	auto node = cond;  // 结果
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond->type == CTNTYPE_LEAF)
	{
		cond->statement = TranslateRegion(condition, blocks[cond->index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!cond->condition)
			throw Exception(_T("翻译为 if 语句的过程中缺少 if 语句的条件表达式"));
		condition = cond->condition;
	}
	CNode* ifCond = condition;
	if (body->type == CTNTYPE_LEAF)
	{
		body->statement = TranslateRegion(condition, blocks[body->index], jumpAddr);
	}
	// 在 if 语句之前还有一段代码
	node->statement = CombineListIf(cond->statement, ifCond, body->statement);
}

void CTranslater::OnReduceIfElse(Node _if, Node t, Node e)
{
	auto cond = ctrees[_if];
	auto then = ctrees[t];
	auto _else = ctrees[e];
	auto node = cond;  // 结果
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond->type == CTNTYPE_LEAF)
	{
		cond->statement = TranslateRegion(condition, blocks[cond->index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!cond->condition)
			throw Exception(_T("翻译为 if - else 语句的过程中缺少 if 语句的条件表达式"));
		condition = cond->condition;
	}
	CNode* ifCond = condition;
	// 需要根据跳转地址来判断哪个基本块是 then 部分，哪个是 else 部分
	if (jumpAddr == blocks[then->index]->GetStartAddress())
	{
		// 这种情况，需要交换 then 和 else 部分
		std::swap(node->_if.then, node->_if._else);
		std::swap(then, _else);
	}
	if (then->type == CTNTYPE_LEAF)
	{
		then->statement = TranslateRegion(condition, blocks[then->index], jumpAddr);
	}
	if (_else->type == CTNTYPE_LEAF)
	{
		_else->statement = TranslateRegion(condition, blocks[_else->index], jumpAddr);
	}
	// 在 if 语句之前还有一段代码
	node->statement = CombineListIf(cond->statement, ifCond, then->statement, _else->statement);
}

// a -> b, a -> c, b ->c, b -> d, c -> d 翻译为 if (x || y) { c }
// 其中 a 包含 条件 x，b 包含条件 y， c是条件满足时要执行的
void CTranslater::OnReduceIfOr(Node _if, Node then, Node _else)
{
	auto a = ctrees[_if];
	auto b = ctrees[then];
	auto c = ctrees[_else];
	auto node = a;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	CNode* condition1 = nullptr, *condition2 = nullptr;
	if (a->type == CTNTYPE_LEAF)
	{
		a->statement = TranslateRegion(condition1, blocks[a->index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!a->condition)
			throw Exception(_T("翻译为 if - or 语句的过程中缺少 if 语句的第1个条件表达式"));
		condition1 = a->condition;
	}
	// 需要根据跳转地址来判断哪个基本块是 then 部分，哪个是 else 部分
	if (jumpAddr == blocks[b->index]->GetStartAddress())
	{
		// 这种情况，需要交换 then 和 else 部分
		std::swap(node->_if.then, node->_if._else);
		std::swap(b, c);
	}
	if (b->type == CTNTYPE_LEAF)
	{
		b->statement = TranslateRegion(condition2, blocks[b->index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!b->condition)
			throw Exception(_T("翻译为 if 语句的过程中缺少 if 语句的第2个条件表达式"));
		condition2 = b->condition;
	}
	// 用 || 连接 a 和 b 的条件，b的条件要取反，因为b条件满足时跳转到d
	condition2 = GetNotExpression(condition2);
	condition1 = allocator.New<CNode>(CNodeKind::EXPR_OR, condition1, condition2);
	if (c->type == CTNTYPE_LEAF)
	{
		c->statement = TranslateRegion(condition2, blocks[c->index], jumpAddr);
	}

	// 多出的那条边翻译为 goto 语句，多出的边的尾节点只有一个后继，所以不会给condition赋值
	//auto name = GetLabelName(blocks[c->index]->GetStartAddress());
	//auto label = allocator.New<CLabelStatement>(name.c_str(), c->statement);  // 尾节点的语句替换为标签语句
	//c->statement = label;

	//auto gotoStat = allocator.New<CGotoStatement>(label);
	//b->statement = CombineListIf(b->statement, condition, b->statement, c->statement);  // 头节点的末尾加上一个条件跳转语句

	// 在 if 语句之前还有一段代码
	node->statement = CombineListIf(a->statement, condition1, c->statement);
}

const Variable* CTranslater::GetLocalVariable(String* name, Type* type)
{
	auto v = this->function->GetVariable(name);
	if (v)
		return v;

	// 添加
	auto variable = allocator.New<Variable>();
	variable->name = name;
	variable->type = type;
	this->function->AddVariable(variable);
	return variable;
}

const Variable* CTranslater::GetLocalVariable(int index)
{
	for (auto v = this->function->GetVariableList(); v; v = v->next)
	{
		if (index-- == 0)
			return v;
	}
	return nullptr;
}

Node CTranslater::CReduce(Node parent, vector<Node> children, CtrlTreeNodeType type)
{
	ControlTreeNodeEx* ctNode = ctrees[parent];
	switch (type)
	{
	case CTNTYPE_LEAF:
		throw Exception(_T("不能将区域归约为叶子区域"));
		break;
	}
	ctNode->type = type;
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

Node CTranslater::CreateBasicBlock()
{
	if (blockCount >= MAX_NODE)
		throw Exception(_T("基本块数量过多"));
	blocks[blockCount].index = blockCount;
	return blockCount++;
}

Node CTranslater::CreateControlTreeNode()
{
	if (controlTreeNodeCount >= MAX_NODE)
		throw Exception(_T("控制树节点数量过多"));
	ctrees[controlTreeNodeCount] = tempAllocator.New<ControlTreeNodeEx>();
	ctrees[controlTreeNodeCount]->index = controlTreeNodeCount;
	return controlTreeNodeCount++;
}

Node CTranslater::ReduceRegionList(NodeSet& N, Node a, Node b)
{
	// r 的前驱是 a 的前驱

	// r 的后继是 b 的后继
	ctrees[a]->succ = ctrees[b]->succ;
	for (auto s : ctrees[b]->Succ())
		ctrees[s]->pred.Replace(b, a);

	// 使用 r 代替 a, b
	N -= b;

	OnReduceList(a, b);
	ctrees[a]->type = CTNTYPE_LIST;

	return CReduce(a, { a, b }, CTNTYPE_LIST);
}

Node CTranslater::ReduceRegionSelfLoop(NodeSet& N, Node a)
{
	// r 的前驱是 a 除了 a 之外的前驱
	ctrees[a]->pred -= a;

	// r 的后继是 a 除了 a 之外的后继
	ctrees[a]->succ -= a;

	// 使用 r 代替 a
	OnReduceSelfLoop(a);
	ctrees[a]->type = CTNTYPE_SELF_LOOP;

	return CReduce(a, { a }, CTNTYPE_SELF_LOOP);
}

Node CTranslater::ReduceRegionIfElse(NodeSet& N, Node a, Node b, Node c)
{
	Node r = a;
	// r 的前驱是 a 的前驱

	// r 的后继是 b 和 c 的后继，且 b 和 c 都只有一个相同的后继
	ctrees[r]->succ = ctrees[b]->succ;
	for (auto s : ctrees[b]->Succ())
	{
		ctrees[s]->pred.Replace({ b, c }, { r });
	}

	// 使用 r 代替 a, b, c
	N -= b;
	N -= c;
	OnReduceIfElse(a, b, c);
	ctrees[r]->type = CTNTYPE_IF_ELSE;

	return CReduce(r, { a, b, c }, CTNTYPE_IF_ELSE);
}

Node CTranslater::ReduceRegionIfOr(NodeSet& N, Node a, Node b, Node c)
{
	Node r = a;
	// r 的前驱是 a 的前驱

	// r 的后继是 b 和 c 的后继 d，且 b -> c, b -> d, c -> d
	ctrees[r]->succ = ctrees[c]->succ;
	for (auto s : ctrees[r]->Succ())
	{
		ctrees[s]->pred.Replace({ b, c }, { r });
	}

	// 使用 r 代替 a, b, c
	N -= b;
	N -= c;

	OnReduceIfOr(a, b, c);
	ctrees[r]->type = CTNTYPE_IF_OR;

	return CReduce(r, { a, b, c }, CTNTYPE_IF_OR);
}

Node CTranslater::ReduceRegionIf(NodeSet& N, Node a, Node b)
{
	Node r = a;
	// r 的前驱是 a 的前驱

	// r 的后继是 a 和 b 的后继，a 只有 b, c 两个后继，b 只有 c 一个后继
	ctrees[r]->succ = ctrees[b]->succ;
	for (auto s : ctrees[b]->Succ())
	{
		ctrees[s]->pred.Replace({ a, b }, { r });
	}

	// 使用 r 代替 a, b
	N -= b;

	OnReduceIf(a, b);
	ctrees[r]->type = CTNTYPE_IF;

	return CReduce(r, { a, b }, CTNTYPE_IF);
}

Node CTranslater::ReduceRegionPoint2Loop(NodeSet& N, Node a, Node b)
{
	Node r = a;
	// r 的前驱是 a 除了 b 之外的前驱
	ctrees[r]->pred = ctrees[a]->pred - b;

	// a 的除 b 之外的后继翻译为 goto 语句
	for (auto s : ctrees[a]->Succ())
	{
		if (s == b)
			continue;
		// a goto s 这条边的goto应该是叶子节点到叶子节点的边
		// COUT << a << " goto " << s << endl;
		// 移除这条边
		ctrees[a]->succ -= s;
		ctrees[s]->pred -= a;
		// GetLeafEdges({ a, s });
		/*SetSub(ctrees[s]->pred, a);
		SetUnion(ctrees[s]->pred, r);*/
	}

	// r 的后继是 b 除了 a 之外的后继
	ctrees[r]->succ = ctrees[b]->succ - a;
	for (auto s : ctrees[r]->Succ())
		ctrees[s]->pred.Replace(b, r);

	// 替换 a，b 为 r
	N -= b;

	OnReducePoint2Loop(a, b);

	return CReduce(r, { a, b }, CTNTYPE_P2LOOP);
}

void CTranslater::GetLeafEntry(ControlTreeNodeEx* node, vector<Node>& nodes)
{
	switch (node->type)
	{
	case CTNTYPE_LEAF:
		nodes.push_back(node->index);
		break;
	case CTNTYPE_LIST:
	case CTNTYPE_P2LOOP:
		GetLeafEntry(node->pair.first, nodes);
		break;
	case CTNTYPE_SELF_LOOP:
		GetLeafEntry(node->node, nodes);
		break;
	case CTNTYPE_IF:
		GetLeafEntry(node->_if.condition, nodes);
		break;
	case CTNTYPE_IF_ELSE:
		GetLeafEntry(node->_if.condition, nodes);
		break;
	default:
		throw Exception(_T("获取叶子入口失败"));
	}
}

void CTranslater::GetLeafExit(ControlTreeNodeEx* node, vector<Node>& nodes)
{
	switch (node->type)
	{
	case CTNTYPE_LEAF:
		nodes.push_back(node->index);
		break;
	case CTNTYPE_LIST:
	case CTNTYPE_P2LOOP:
		GetLeafEntry(node->pair.second, nodes);
		break;
	case CTNTYPE_SELF_LOOP:
		GetLeafEntry(node->node, nodes);
		break;
	case CTNTYPE_IF:
		GetLeafEntry(node->_if.condition, nodes);
		GetLeafEntry(node->_if.then, nodes);
		break;
	case CTNTYPE_IF_ELSE:
		GetLeafEntry(node->_if.then, nodes);
		GetLeafEntry(node->_if._else, nodes);
		break;
	default:
		throw Exception(_T("获取叶子出口失败"));
	}
}

std::vector<Edge> CTranslater::GetLeafEdges(Edge e)
{
	vector<Edge> result(32);
	result.clear();
	vector<Node> entries;
	vector<Node> exits;
	GetLeafEntry(ctrees[e.from], entries);
	GetLeafExit(ctrees[e.to], exits);
	for (auto from : entries)
	{
		for (auto to : exits)
		{
			result.push_back({ from, to });
		}
	}
	return result;
}

// 基本块节点转换为控制树节点
void CTranslater::InitializeBaseControlTree(NodeSet N)
{
	//memset(ctrees, 0, sizeof(ctrees));
	for (auto n : Nodes(N))
	{
		ctrees[n] = tempAllocator.New<ControlTreeNodeEx>();
		*(BasicBlock*)ctrees[n] = blocks[n];
		ctrees[n]->type = CTNTYPE_LEAF;
	}
	controlTreeNodeCount = blockCount;
}


NodeSet CTranslater::CAnalysis(NodeSet N)
{
	// 首先将基本块构成的控制流图转换为控制树节点构成的控制流图
	InitializeBaseControlTree(N);

	while (true)
	{
		for (auto n : Nodes(N))
		{
			switch (ctrees[n]->GetSuccCount())
			{
			case 1:
			{
					  Node succ = ctrees[n]->Succ()[0];
					  if (ctrees[succ]->GetPredCount() == 1)
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
					  auto succ = ctrees[n]->Succ();
					  auto b = ctrees[succ[0]];
					  auto c = ctrees[succ[1]];
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
			for (auto s : ctrees[n]->Succ())
			{
				if (s == n)  // 自循环检测
				{
					//DumpCurrentCFG(N);
					ReduceRegionSelfLoop(N, n);
					//DumpCurrentCFG(N);
					goto NEXT;
				}
				// 两点循环 a -> b && b -> a 并且 b 只有一个前驱
				if (ctrees[s]->succ.Contains(n) && ctrees[s]->GetPredCount() == 1)
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

void CTranslater::DumpCFG()
{
	for (int i = 0; i < blockCount; ++i)
	{
		auto block = &blocks[i];
		COUT << _T("block ") << i << _T(" , 前驱 : ");
		DumpNodeSet(block->pred);
		COUT << _T(" 后继 : ");
		DumpNodeSet(block->succ);
		COUT << endl;
	}
}

void CTranslater::DumpControlTree()
{
	for (int i = 0; i < controlTreeNodeCount; ++i)
	{
		auto block = ctrees[i];
		COUT << _T("tree node ") << i << _T(" , 前驱 : ");
		DumpNodeSet(block->pred);
		COUT << _T(" 后继 : ");
		DumpNodeSet(block->succ);
		COUT << endl;
	}
}

void CTranslater::DumpCurrentCFG(NodeSet& N)
{
	auto nodes = Nodes(N);
	for (auto i : nodes)
	{
		auto node = ctrees[i];
		COUT << _T("tree node ") << i << _T(" , 前驱 : ");
		DumpNodeSet(node->pred);
		COUT << _T(" 后继 : ");
		DumpNodeSet(node->succ);
		COUT << endl;
	}
}
