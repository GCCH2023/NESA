#include "stdafx.h"
#include "CTranslater.h"
using namespace std;

CTranslater::CTranslater(Allocator& allocator_, NesDataBase& db_) :
db(db_),
allocator(allocator_),
function(nullptr),
subroutine(nullptr)
{
	registers[Nes::NesRegisters::A] = allocator.New<CNode>(NewString(_T("A")));
	registers[Nes::NesRegisters::X] = allocator.New<CNode>(NewString(_T("X")));
	registers[Nes::NesRegisters::Y] = allocator.New<CNode>(NewString(_T("Y")));
	registers[Nes::NesRegisters::P] = allocator.New<CNode>(NewString(_T("P")));
	registers[Nes::NesRegisters::SP] = allocator.New<CNode>(NewString(_T("SP")));

	noneStatement = allocator.New<CNode>(CNodeKind::STAT_NONE);
}


CTranslater::~CTranslater()
{
}

Function* CTranslater::TranslateSubroutine(TACSubroutine* subroutine)
{
	if (!subroutine)
		return nullptr;
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
	auto root = Analyze(edges.data(), edges.size());
	// 遍历控制树，生成C语句
	// COUT << root->statement;
	Function* func = allocator.New<Function>();
	func->SetBody(root->statement);
	TCHAR buffer[64];
	_stprintf_s(buffer, _T("sub_%04X"), subroutine->GetStartAddress());
	func->name = buffer;
	return func;
}

ControlTreeNodeEx* CTranslater::Analyze(Edge edges[], size_t count)
{
	BuildCFG(edges, count);
	NodeSet N = CAnalysis(GetFullSet());
	if (N.GetSize() != 1)  // 也可能只有一个基本块
	{
		DumpCFG();
		throw Exception(_T("控制树无法归约到单一根节点"));
	}
	// 在全部语句都生成后，回填标签语句
	PatchLabels();

	Node n = N.ToVector()[0];
	if (ctrees[n].statement == nullptr)
	{
		CNode* condition = nullptr;
		uint32_t jumpAddr;
		auto block = this->subroutine->GetBasicBlocks()[n];
		ctrees[n].statement = TranslateRegion(condition, block, jumpAddr);
	}
	return &ctrees[n];
}

void CTranslater::BuildCFG(Edge edges[], size_t count)
{
	Reset();

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
	blockCount = 0;
}

CNode* CTranslater::GetExpression(TACOperand& operand)
{
	switch (operand.GetKind())
	{
	case TACOperand::INTEGER:
		if (operand.IsTemp())
		{
			return allocator.New<CNode>(NewString(_T("temp%d"), operand.GetValue()), VAR_KIND_LOCAL);
		}
		else
			return allocator.New<CNode>(operand.GetValue());
	case TACOperand::REGISTER:
		return registers[operand.GetValue()];
	case TACOperand::MEMORY:
	{
							   if (operand.IsTemp())
							   {
								   auto var = allocator.New<CNode>(NewString(_T("temp%d"), operand.GetValue()), VAR_KIND_GLOBAL);
								   // 需要解引用
								   return allocator.New<CNode>(CNodeKind::EXPR_REF, var);
							   }
							   return allocator.New<CNode>(NewString(_T("g_%04X"), operand.GetValue()), VAR_KIND_GLOBAL);
	}
	case TACOperand::ADDRESS:
	{
								return allocator.New<CNode>(NewString(_T("g_%04X"), operand.GetValue()), VAR_KIND_GLOBAL);
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


CNode* CTranslater::TranslateRegion(CNode*& pCondition, TACBasicBlock* tacBlock, uint32_t& jumpAddr)
{
	CNode* current = nullptr, *last = nullptr, *tail = nullptr;
	CNode* expr = nullptr;
	CNode* list = nullptr;
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
									auto call = allocator.New<CNode>(NewString(_T("sub_%04X"), codes[i]->z.GetValue()), params);
									current = call;
									break;
		}
		case	TACOperator::CALL:
		{
									 // 如果有参数，则鄙视 若干个 ARG 后面跟着一个 CALL
									 // 直接出现 CALL，说明没有参数
									 current = allocator.New<CNode>(NewString(_T("sub_%04X"), tac->z.GetValue()));
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
								  // goto 在控制流图中对应一条边，可能被处理成循环结构，也可能就是对应goto语句，
								  // 还不知道该怎么处理
								  if (tac->z.GetValue() == tac->address)
								  {
									  // 跳转到自己的语句翻译为 while (1);
									  //expr = allocator.New<CInteger>(1);
									  //current = allocator.New<CWhileStatement>(expr, noneStatement);
									  current = noneStatement;
									  pCondition = allocator.New<CNode>(1);
									  break;
								  }
								  auto label = GetLabelName(tac->z.GetValue());
								  current = allocator.New<CNode>(CNodeKind::STAT_GOTO, label);
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
		default:
		{
				   TCHAR buffer[64];
				   _stprintf_s(buffer, _T("三地址码转C语句：未实现的三地址码 %s"), ToString(tac->op));
				   throw Exception(buffer);
		}
		}
		// 构建语句列表
		if (tail)
		{
			tail->e.y = allocator.New<CNode>(CNodeKind::STAT_LIST, tail->e.y, current);
			tail = tail->e.y;
		}
		else if (last)
		{
			tail = list = allocator.New<CNode>(CNodeKind::STAT_LIST, last, current);
		}
		last = current;
	}
	auto ret = list ? list : current;
	blockStatements[tacBlock->GetStartAddress()] = ret;  // 记录下这个基本块对应的地址及语句
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


CStr CTranslater::GetLabelName(uint32_t jumpAddr)
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
	return allocator.New<CNode>(CNodeKind::STAT_LIST, statement, ifStat);
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

CStr CTranslater::NewString(const CStr format, ...)
{
	TCHAR buffer[256];
	va_list va;
	va_start(va, format);
	_vstprintf_s(buffer, 256, format, va);
	va_end(va);
	size_t count = ((_tcslen(buffer) + 1) * sizeof(TCHAR)+sizeof(void*)-1) & ~(sizeof(void*)-1);
	CStr str = allocator.Alloc<TCHAR>(count);
	_tcscpy_s(str, count, buffer);
	return str;
}

// 当要将控制流图中的一个自循环节点归约时
// a -> a
void CTranslater::OnReduceSelfLoop(ControlTreeNodeEx* node)
{
	auto body = node->node;
	if (body->type != CTNTYPE_LEAF)
	{
		if (body->condition == nullptr)
			throw Exception(_T("非叶子自循环节点异常"));
		node->statement = allocator.New<CNode>(CNodeKind::STAT_DO_WHILE, body->condition, body->statement);
		return;
	}
	CNode* condition = nullptr;
	auto block = this->subroutine->GetBasicBlocks()[body->index];
	uint32_t jumpAddr;
	auto a = TranslateRegion(condition, block, jumpAddr);
	node->statement = allocator.New<CNode>(CNodeKind::STAT_DO_WHILE, condition, a);
}

void CTranslater::OnReduceList(ControlTreeNodeEx* node)
{
	auto first = node->pair.first;
	auto second = node->pair.second;
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
	node->statement = allocator.New<CNode>(CNodeKind::STAT_LIST, first->statement, second->statement);
	node->condition = condition;
}

void CTranslater::OnReducePoint2Loop(ControlTreeNodeEx* node)
{
	auto first = node->pair.first;
	auto second = node->pair.second;
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
		throw Exception(_T("未实现"));
	}
	if (second->type == CTNTYPE_LEAF)
	{
		second->statement = TranslateRegion(condition, blocks[second->index], jumpAddr);
	}
	else
	{
		throw Exception(_T("未实现"));
	}
	node->statement = allocator.New<CNode>(CNodeKind::STAT_LIST, first->statement, second->statement);
	node->statement = allocator.New<CNode>(CNodeKind::STAT_DO_WHILE, condition, node->statement);
}

void CTranslater::OnReduceIf(ControlTreeNodeEx* node)
{
	auto cond = node->_if.condition;
	auto body = node->_if.then;
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond->type == CTNTYPE_LEAF)
	{
		cond->statement = TranslateRegion(condition, blocks[cond->index], jumpAddr);
	}
	else
	{
		throw Exception(_T("未实现"));
	}
	CNode* ifCond = condition;
	if (body->type == CTNTYPE_LEAF)
	{
		body->statement = TranslateRegion(condition, blocks[body->index], jumpAddr);
	}
	// 在 if 语句之前还有一段代码
	node->statement = CombineListIf(cond->statement, ifCond, body->statement);
}

void CTranslater::OnReduceIfElse(ControlTreeNodeEx* node)
{
	auto cond = node->_if.condition;
	auto then = node->_if.then;
	auto _else = node->_if._else;
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond->type == CTNTYPE_LEAF)
	{
		cond->statement = TranslateRegion(condition, blocks[cond->index], jumpAddr);
	}
	else
	{
		throw Exception(_T("未实现"));
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
void CTranslater::OnReduceIfOr(ControlTreeNodeEx* node)
{
	auto a = node->_if.condition;
	auto b = node->_if.then;
	auto c = node->_if._else;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	CNode* condition1 = nullptr, *condition2 = nullptr;
	if (a->type == CTNTYPE_LEAF)
	{
		a->statement = TranslateRegion(condition1, blocks[a->index], jumpAddr);
	}
	else
	{
		throw Exception(_T("未实现"));
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
	else if (b == b)
	{
		throw Exception(_T("未实现"));
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

Node CTranslater::CReduce(Node parent, vector<Node> children, CtrlTreeNodeType type)
{
	ControlTreeNodeEx* ctNode = &ctrees[parent];
	ctNode->type = type;
	ctNode->index = parent;
	switch (type)
	{
	case CTNTYPE_LEAF:
		throw Exception(_T("不能将区域归约为叶子区域"));
		break;
	case CTNTYPE_LIST:
		ctNode->pair.first = &ctrees[children[0]];
		ctNode->pair.second = &ctrees[children[1]];
		OnReduceList(ctNode);
		break;
	case CTNTYPE_P2LOOP:
		ctNode->pair.first = &ctrees[children[0]];
		ctNode->pair.second = &ctrees[children[1]];
		OnReducePoint2Loop(ctNode);
		break;
	case CTNTYPE_SELF_LOOP:
		ctNode->node = &ctrees[children[0]];
		OnReduceSelfLoop(ctNode);
		break;
	case CTNTYPE_IF:
		ctNode->_if.condition = &ctrees[children[0]];
		ctNode->_if.then = &ctrees[children[1]];
		OnReduceIf(ctNode);
		break;
	case CTNTYPE_IF_ELSE:
		ctNode->_if.condition = &ctrees[children[0]];
		ctNode->_if.then = &ctrees[children[1]];
		ctNode->_if._else = &ctrees[children[2]];
		OnReduceIfElse(ctNode);
		break;
	case CTNTYPE_IF_OR:
		ctNode->_if.condition = &ctrees[children[0]];
		ctNode->_if.then = &ctrees[children[1]];
		ctNode->_if._else = &ctrees[children[2]];
		OnReduceIfOr(ctNode);
		break;
	}
	//COUT << "归约 " << ToString(type) << " " << parent << " : ";
	//for (auto n : children)
	//	COUT << n << ", ";
	//COUT << endl;
	//ctNode->Dump();
	//COUT << ctNode->statement;
	//COUT << endl;
	return parent;
}

Node CTranslater::Create_Node()
{
	if (blockCount >= 32)
		throw Exception(_T("基本块数量过多"));
	blocks[blockCount].index = blockCount;
	return blockCount++;
}

Node CTranslater::ReduceRegionList(NodeSet& N, Node a, Node b)
{
	Node r = Create_Node();
	// r 的前驱是 a 的前驱
	ctrees[r].pred = ctrees[a].pred;
	for (auto pred : ctrees[a].Pred())
		ctrees[pred].succ.Replace(a, r);

	// r 的后继是 b 的后继
	ctrees[r].succ = ctrees[b].succ;
	for (auto s : ctrees[b].Succ())
		ctrees[s].pred.Replace(b, r);

	// 使用 r 代替 a, b
	N.Replace({ a, b }, { r });
	return CReduce(r, { a, b }, CTNTYPE_LIST);
}

Node CTranslater::ReduceRegionSelfLoop(NodeSet& N, Node a)
{
	Node r = Create_Node();
	// r 的前驱是 a 除了 a 之外的前驱
	ctrees[r].pred = ctrees[a].pred - a;
	for (auto pred : ctrees[r].Pred())
		ctrees[pred].succ.Replace(a, r);

	// r 的后继是 a 除了 a 之外的后继
	ctrees[r].succ = ctrees[a].succ - a;
	for (auto s : ctrees[r].Succ())
		ctrees[s].pred.Replace(a, r);

	// 使用 r 代替 a
	N.Replace(a, r);
	return CReduce(r, { a }, CTNTYPE_SELF_LOOP);
}

Node CTranslater::ReduceRegionIfElse(NodeSet& N, Node a, Node b, Node c)
{
	Node r = Create_Node();
	// r 的前驱是 a 的前驱
	ctrees[r].pred = ctrees[a].pred;
	for (auto pred : ctrees[a].Pred())
		ctrees[pred].succ.Replace(a, r);

	// r 的后继是 b 和 c 的后继，且 b 和 c 都只有一个相同的后继
	ctrees[r].succ = ctrees[b].succ;
	for (auto s : ctrees[b].Succ())
	{
		ctrees[s].pred.Replace({ b, c }, { r });
	}

	// 使用 r 代替 a, b, c
	N.Replace({ a, b, c }, { r });
	return CReduce(r, { a, b, c }, CTNTYPE_IF_ELSE);
}

Node CTranslater::ReduceRegionIfOr(NodeSet& N, Node a, Node b, Node c)
{
	Node r = Create_Node();
	// r 的前驱是 a 的前驱
	ctrees[r].pred = ctrees[a].pred;
	for (auto pred : ctrees[a].Pred())
		ctrees[pred].succ.Replace(a, r);

	// r 的后继是 b 和 c 的后继 d，且 b -> c, b -> d, c -> d
	ctrees[r].succ = ctrees[c].succ;
	for (auto s : ctrees[r].Succ())
	{
		ctrees[s].pred.Replace({ b, c }, { r });
	}

	// 使用 r 代替 a, b, c
	N.Replace({ a, b, c }, { r });
	return CReduce(r, { a, b, c }, CTNTYPE_IF_OR);
}

Node CTranslater::ReduceRegionIf(NodeSet& N, Node a, Node b)
{
	Node r = Create_Node();
	// r 的前驱是 a 的前驱
	ctrees[r].pred = ctrees[a].pred;
	for (auto pred : ctrees[a].Pred())
		ctrees[pred].succ.Replace(a, r);

	// r 的后继是 a 和 b 的后继，且 a 和 b 都只有一个相同的后继
	ctrees[r].succ = ctrees[b].succ;
	for (auto s : ctrees[b].Succ())
	{
		ctrees[s].pred.Replace({ a, b }, { r });
	}

	// 使用 r 代替 a, b
	N.Replace({ a, b }, { r });
	return CReduce(r, { a, b }, CTNTYPE_IF);
}

Node CTranslater::ReduceRegionPoint2Loop(NodeSet& N, Node a, Node b)
{
	//COUT << "2点循环:\n";
	//ctrees[a].pred.Dump();
	//COUT << endl;
	//ctrees[b].succ.Dump();
	//COUT << endl;
	Node r = Create_Node();
	// r 的前驱是 a 除了 b 之外的前驱
	ctrees[r].pred = ctrees[a].pred - b;

	for (auto pred : ctrees[r].Pred())
		ctrees[pred].succ.Replace(a, r);

	// r 的后继是 b 除了 a 之外的后继
	ctrees[r].succ = ctrees[b].succ - a;
	for (auto s : ctrees[r].Succ())
		ctrees[s].pred.Replace(b, r);

	// a 的除 b 之外的后继翻译为 goto 语句
	for (auto s : ctrees[a].Succ())
	{
		if (s == b)
			continue;
		// a goto s 这条边的goto应该是叶子节点到叶子节点的边
		// COUT << a << " goto " << s << endl;
		// 移除这条边
		ctrees[a].succ -= s;
		ctrees[s].pred -= a;
		// GetLeafEdges({ a, s });
		/*SetSub(ctrees[s].pred, a);
		SetUnion(ctrees[s].pred, r);*/
	}

	// 替换 a，b 为 r
	N.Replace({ a, b }, { r });
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
	GetLeafEntry(&ctrees[e.from], entries);
	GetLeafExit(&ctrees[e.to], exits);
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
		*(BasicBlock*)&ctrees[n] = blocks[n];
		ctrees[n].type = CTNTYPE_LEAF;
	}
}


NodeSet CTranslater::CAnalysis(NodeSet N)
{
	// 首先将基本块构成的控制流图转换为控制树节点构成的控制流图
	InitializeBaseControlTree(N);

	NodeSet old = 0;
	while (true)
	{
		if (old == N)
		{
			break;
		}
		old = N;
		for (auto n : Nodes(N))
		{
			switch (ctrees[n].GetSuccCount())
			{
			case 1:
			{
					  Node succ = ctrees[n].Succ()[0];
					  if (ctrees[succ].GetPredCount() == 1)
					  {
						  ReduceRegionList(N, n, succ);
						  // 下一次循环
						  goto NEXT;
					  }
					  break;
			}
			case 2:
			{
					  auto succ = ctrees[n].Succ();
					  auto b = &ctrees[succ[0]];
					  auto c = &ctrees[succ[1]];
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
						  goto NEXT;
					  }
					  if (c->GetPredCount() == 1 && c->GetSuccCount() == 1 &&
						  c->succ.Contains(b->index))
					  {
						  ReduceRegionIf(N, n, c->index);
						  goto NEXT;
					  }
					  // 检测 if

					  break;
			}
			}
			// 循环检测
			for (auto s : ctrees[n].Succ())
			{
				if (s == n)  // 自循环检测
				{
					ReduceRegionSelfLoop(N, n);
					goto NEXT;
				}
				// 两点循环 a -> b && b -> a 并且 b 只有一个前驱
				if (ctrees[s].succ.Contains(n) && ctrees[s].GetPredCount() == 1)
				{
					ReduceRegionPoint2Loop(N, n, s);
					goto NEXT;
				}
			}
		}

	NEXT:
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
		block->pred.Dump();
		COUT << _T(" 后继 : ");
		block->succ.Dump();
		COUT << endl;
	}
}