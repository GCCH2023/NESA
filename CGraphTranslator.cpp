#include "stdafx.h"
#include "CGraphTranslator.h"
using namespace std;
#include "Function.h"
#include "CDataBase.h"
#include "CBasicBlockTranslator.h"


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
	auto& blocks = GetTACFunction()->GetBasicBlocks();
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
#ifdef DEBUG_GRAPH
	Sprintf<> s;
	for (size_t i = 0; i < blocks.size(); ++i)
	{
		COUT << s.Format(_T("%04X - %d\n"), blocks[i]->GetStartAddress(), i);
	}
#endif


	graph.Resize(blocks.size());
	graph.AddEdges(edges);
}

void CGraphTranslator::Reset()
{
	CTranslator::Reset();

	this->graph.Clear();
	tempAllocator.Reset();
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

CNode* CGraphTranslator::TranslateRegion(CNode*& condition, TACBasicBlock* tacBlock, uint32_t& jumpAddr)
{
	CBasicBlockTranslator translator(this);
	auto node = translator.Translate(tacBlock);
	condition = translator.GetCondition();
	jumpAddr = translator.GetJumpTarget();
	return node;
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
	assert(first->GetNext() == nullptr);
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
	//		first->list.tail->AddNext(second->list.head;
	//		first->list.tail = second->list.tail;
	//		return first;
	//	}
	//	// 添加到末尾
	//	first->list.tail->AddNext(second;
	//	first->list.tail = second;
	//	return first;
	//}
	//else if (second->kind == CNodeKind::STAT_LIST)
	//{
	//	// 添加到开头
	//	first->AddNext(second->list.head;
	//	second->list.head = first;
	//	return second;
	//}
	first->SetNext(second);
	return NewStatementList(first, second);
}




// 当要将控制流图中的一个自循环节点归约时
// a -> a
void CGraphTranslator::OnReduceSelfLoop(Node n)
{
	auto& node = graph[n];
	if (node.type != CTNTYPE_LEAF)
	{
		if (node.condition == nullptr)
			throw Exception(_T("非叶子自循环节点异常"));
		node.statement = NewDoWhile(node.condition, node.statement);
		return;
	}
	CNode* condition = nullptr;
	auto block = this->GetTACFunction()->GetBasicBlocks()[node.index];
	uint32_t jumpAddr;
	auto a = TranslateRegion(condition, block, jumpAddr);
	node.statement = NewDoWhile(condition, a);
}

void CGraphTranslator::OnReduceList(Node f, Node s)
{
	auto& first = graph[f];
	auto& second = graph[s];
	CNode* condition = nullptr;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	if (first.type == CTNTYPE_LEAF)
	{
		first.statement = TranslateRegion(condition, blocks[first.index], jumpAddr);
	}
	if (second.type == CTNTYPE_LEAF)
	{
		second.statement = TranslateRegion(condition, blocks[second.index], jumpAddr);
	}
	first.statement = NewStatementPair(first.statement, second.statement);
	first.condition = condition;
}

void CGraphTranslator::OnReducePoint2Loop(Node f, Node s)
{
	auto& first = graph[f];
	auto& second = graph[s];
	auto& node = first;  // 结果
	CNode* condition = nullptr;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	if (first.type == CTNTYPE_LEAF)
	{
		first.statement = TranslateRegion(condition, blocks[first.index], jumpAddr);
		// 跳转边翻译为 goto 语句
		auto gotoStat = allocator.New<CNode>(CNodeKind::STAT_GOTO, GetLabelName(jumpAddr));
		first.statement = CombineListIf(first.statement, condition, gotoStat);
	}
	else
	{
		throw Exception(_T("2点循环的第一个节点不是叶子节点的归约未实现"));
	}
	if (second.type == CTNTYPE_LEAF)
	{
		second.statement = TranslateRegion(condition, blocks[second.index], jumpAddr);
	}
	else
	{
		assert(second.statement);
		assert(second.condition);
		condition = second.condition;
	}
	node.statement = NewStatementPair(first.statement, second.statement);
	node.statement = NewDoWhile(condition, node.statement);
}

void CGraphTranslator::OnReduceIf(Node _if, Node then)
{
	auto& cond = graph[_if];
	auto& body = graph[then];
	auto& node = cond;  // 结果
	CNode* condition = nullptr;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond.type == CTNTYPE_LEAF)
	{
		cond.statement = TranslateRegion(condition, blocks[cond.index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!cond.condition)
			throw Exception(_T("翻译为 if 语句的过程中缺少 if 语句的条件表达式"));
		condition = cond.condition;
	}
	CNode* ifCond = condition;
	if (body.type == CTNTYPE_LEAF)
	{
		body.statement = TranslateRegion(condition, blocks[body.index], jumpAddr);
	}
	// 在 if 语句之前还有一段代码
	node.statement = CombineListIf(cond.statement, ifCond, body.statement);
}

void CGraphTranslator::OnReduceIfElse(Node _if, Node t, Node e)
{
	auto& cond = graph[_if];
	auto& then = graph[t];
	auto& _else = graph[e];
	auto& node = cond;  // 结果
	CNode* condition = nullptr;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond.type == CTNTYPE_LEAF)
	{
		cond.statement = TranslateRegion(condition, blocks[cond.index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!cond.condition)
			throw Exception(_T("翻译为 if - else 语句的过程中缺少 if 语句的条件表达式"));
		condition = cond.condition;
	}
	CNode* ifCond = condition;
	// 需要根据跳转地址来判断哪个基本块是 then 部分，哪个是 else 部分
	if (jumpAddr == blocks[then.index]->GetStartAddress())
	{
		// 这种情况，需要交换 then 和 else 部分
		std::swap(node._if.then, node._if._else);
		std::swap(then, _else);
	}
	if (then.type == CTNTYPE_LEAF)
	{
		then.statement = TranslateRegion(condition, blocks[then.index], jumpAddr);
	}
	if (_else.type == CTNTYPE_LEAF)
	{
		_else.statement = TranslateRegion(condition, blocks[_else.index], jumpAddr);
	}
	// 在 if 语句之前还有一段代码
	node.statement = CombineListIf(cond.statement, ifCond, then.statement, _else.statement);
}

// a -> b, a -> c, b ->c, b -> d, c -> d 翻译为 if (x || y) { c }
// 其中 a 包含 条件 x，b 包含条件 y， c是条件满足时要执行的
void CGraphTranslator::OnReduceIfOr(Node _if, Node then, Node _else)
{
	auto& a = graph[_if];
	auto& b = graph[then];
	auto& c = graph[_else];
	auto& node = a;
	auto blocks = this->GetTACFunction()->GetBasicBlocks();
	uint32_t jumpAddr;
	CNode* condition1 = nullptr, *condition2 = nullptr;
	if (a.type == CTNTYPE_LEAF)
	{
		a.statement = TranslateRegion(condition1, blocks[a.index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!a.condition)
			throw Exception(_T("翻译为 if - or 语句的过程中缺少 if 语句的第1个条件表达式"));
		condition1 = a.condition;
	}
	// 需要根据跳转地址来判断哪个基本块是 then 部分，哪个是 else 部分
	if (jumpAddr == blocks[b.index]->GetStartAddress())
	{
		// 这种情况，需要交换 then 和 else 部分
		std::swap(node._if.then, node._if._else);
		std::swap(b, c);
	}
	if (b.type == CTNTYPE_LEAF)
	{
		b.statement = TranslateRegion(condition2, blocks[b.index], jumpAddr);
	}
	else
	{
		// 如果不是叶子节点，则之前的归约必然要保留有条件
		if (!b.condition)
			throw Exception(_T("翻译为 if 语句的过程中缺少 if 语句的第2个条件表达式"));
		condition2 = b.condition;
	}
	// 用 || 连接 a 和 b 的条件，b的条件要取反，因为b条件满足时跳转到d
	condition2 = GetNotExpression(condition2);
	condition1 = allocator.New<CNode>(CNodeKind::EXPR_OR, condition1, condition2);
	if (c.type == CTNTYPE_LEAF)
	{
		c.statement = TranslateRegion(condition2, blocks[c.index], jumpAddr);
	}

	// 多出的那条边翻译为 goto 语句，多出的边的尾节点只有一个后继，所以不会给condition赋值
	//auto name = GetLabelName(blocks[c.index].GetStartAddress());
	//auto label = allocator.New<CLabelStatement>(name.c_str(), c.statement);  // 尾节点的语句替换为标签语句
	//c.statement = label;

	//auto gotoStat = allocator.New<CGotoStatement>(label);
	//b.statement = CombineListIf(b.statement, condition, b.statement, c.statement);  // 头节点的末尾加上一个条件跳转语句

	// 在 if 语句之前还有一段代码
	node.statement = CombineListIf(a.statement, condition1, c.statement);
}



Node CGraphTranslator::CReduce(Node parent, const vector<Node>& children)
{
#ifdef DEBUG_GRAPH
	auto& node = graph[parent];
	Sprintf<> s;
	COUT << s.Format(_T("归约 %s %d :"), ToString(node.type), parent);
	for (auto n : children)
		COUT << n << _T(", ");
	COUT << endl;

	s.Clear();
	COUT << _T("前驱 = ");
	DumpNodeSet(node.pred);
	COUT << _T(", 后继 = ");
	DumpNodeSet(node.succ);
	COUT << endl;
	//ctNode->Dump();
	//COUT << ctNode->statement;
	//COUT << endl;
#endif
	return parent;
}

Node CGraphTranslator::ReduceRegionList(VertexSet& N, Node a, Node b)
{
	// r 的前驱是 a 的前驱
	// r 的后继是 b 的后继
	graph[a].succ = graph[b].succ;
	for (auto s : graph[b].Succ())
		graph[s].pred.Replace(b, a);

	// 使用 r 代替 a, b
	N -= b;

	OnReduceList(a, b);
	graph[a].type = CTNTYPE_LIST;

	return CReduce(a, { a, b });
}

Node CGraphTranslator::ReduceRegionSelfLoop(VertexSet& N, Node a)
{
	// r 的前驱是 a 除了 a 之外的前驱
	graph[a].pred -= a;

	// r 的后继是 a 除了 a 之外的后继
	graph[a].succ -= a;

	// 使用 r 代替 a
	OnReduceSelfLoop(a);
	graph[a].type = CTNTYPE_SELF_LOOP;

	return CReduce(a, { a });
}

Node CGraphTranslator::ReduceRegionIfElse(VertexSet& N, Node a, Node b, Node c)
{
	Node r = a;
	// r 的前驱是 a 的前驱

	// r 的后继是 b 和 c 的后继，且 b 和 c 都只有一个相同的后继
	graph[r].succ = graph[b].succ;
	for (auto s : graph[b].Succ())
	{
		graph[s].pred -= b;
		graph[s].pred -= c;
		graph[s].pred += r;
	}

	// 使用 r 代替 a, b, c
	N -= b;
	N -= c;
	OnReduceIfElse(a, b, c);
	graph[r].type = CTNTYPE_IF_ELSE;

	return CReduce(r, { a, b, c });
}

Node CGraphTranslator::ReduceRegionIfOr(VertexSet& N, Node a, Node b, Node c)
{
	Node r = a;
	// r 的前驱是 a 的前驱

	// r 的后继是 b 和 c 的后继 d，且 b -> c, b -> d, c -> d
	graph[r].succ = graph[c].succ;
	for (auto s : graph[r].Succ())
	{
		graph[s].pred -= b;
		graph[s].pred -= c;
		graph[s].pred += r;
	}

	// 使用 r 代替 a, b, c
	N -= b;
	N -= c;

	OnReduceIfOr(a, b, c);
	graph[r].type = CTNTYPE_IF_OR;

	return CReduce(r, { a, b, c });
}

Node CGraphTranslator::ReduceRegionIf(VertexSet& N, Node a, Node b)
{
	Node r = a;
	// r 的前驱是 a 的前驱
	// r 的后继是 a 和 b 的后继，a 只有 b, c 两个后继，b 只有 c 一个后继
	graph[r].succ = graph[b].succ;
	for (auto s : graph[b].Succ())
	{
		graph[s].pred -= b;
	}

	// 使用 r 代替 a, b
	N -= b;

	OnReduceIf(a, b);
	graph[r].type = CTNTYPE_IF;

	return CReduce(r, { a, b });
}

Node CGraphTranslator::ReduceRegionPoint2Loop(VertexSet& N, Node a, Node b)
{
	Node r = a;
	// r 的前驱是 a 除了 b 之外的前驱
	graph[r].pred = graph[a].pred - b;

	// a 的除 b 之外的后继翻译为 goto 语句
	for (auto s : graph[a].Succ())
	{
		if (s == b)
			continue;
		// a goto s 这条边的goto应该是叶子节点到叶子节点的边
		// COUT << a << " goto " << s << endl;
		// 移除这条边
		graph[a].succ -= s;
		graph[s].pred -= a;
		// GetLeafEdges({ a, s });
		/*SetSub(graph[s].pred, a);
		SetUnion(graph[s].pred, r);*/
	}

	// r 的后继是 b 除了 a 之外的后继
	graph[r].succ = graph[b].succ - a;
	for (auto s : graph[r].Succ())
		graph[s].pred.Replace(b, r);

	// 替换 a，b 为 r
	N -= b;

	OnReducePoint2Loop(a, b);
	graph[r].type = CTNTYPE_P2LOOP;

	return CReduce(r, { a, b });
}

VertexSet& CGraphTranslator::CAnalysis(VertexSet& N)
{
	while (true)
	{
		for (auto n : Nodes(N))
		{
			switch (graph[n].GetSuccCount())
			{
			case 1:
			{
					  Node succ = graph[n].Succ()[0];
					  if (graph[succ].GetPredCount() == 1)
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
					  auto succ = graph[n].Succ();
					  auto& b = graph[succ[0]];
					  auto& c = graph[succ[1]];
					  // a -> b, a -> c, b -> d, c -> d 归约为 if else 结构
					  if (b.succ == c.succ && b.GetSuccCount() == 1 &&
						  b.GetPredCount() == 1 && c.GetPredCount() == 1)
					  {
						  ReduceRegionIfElse(N, n, b.index, c.index);
						  goto NEXT;
					  }
					  // a -> b, a -> c, b -> d, c -> d, b -> c 归约为 if (x || y)
					  if ((b.succ & c.succ).Any())  // b 和 c 有相同的后继
					  {
						  if (b.GetSuccCount() == 2 && c.GetSuccCount() == 1 && b.succ.Contains(c.index))
						  {
							  // b -> c 的边翻译为 goto
							  ReduceRegionIfOr(N, n, b.index, c.index);
							  goto NEXT;
						  }
						  if (c.GetSuccCount() == 2 && b.GetSuccCount() == 1 && c.succ.Contains(b.index))
						  {
							  ReduceRegionIfOr(N, n, c.index, b.index);
							  // c -> b 的边翻译为 goto
							  goto NEXT;
						  }
					  }

					  if (b.GetPredCount() == 1 && b.GetSuccCount() == 1 &&
						  b.succ.Contains(c.index))
					  {
						  ReduceRegionIf(N, n, b.index);
						  //DumpCurrentCFG(N);
						  goto NEXT;
					  }
					  if (c.GetPredCount() == 1 && c.GetSuccCount() == 1 &&
						  c.succ.Contains(b.index))
					  {
						  ReduceRegionIf(N, n, c.index);
						  //DumpCurrentCFG(N);
						  goto NEXT;
					  }
					  // 检测 if

					  break;
			}
			}
			// 循环检测
			for (auto s : graph[n].Succ())
			{
				if (s == n)  // 自循环检测
				{
					//DumpCurrentCFG(N);
					ReduceRegionSelfLoop(N, n);
					//DumpCurrentCFG(N);
					goto NEXT;
				}
				// 两点循环 a -> b && b -> a 并且 b 只有一个前驱
				if (graph[s].succ.Contains(n) && graph[s].GetPredCount() == 1)
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

	VertexSet N;
	try
	{
		N = this->graph.GetFullSet();
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
	if (graph[n].statement == nullptr)
	{
		CNode* condition = nullptr;
		uint32_t jumpAddr;
		auto block = GetTACFunction()->GetBasicBlocks()[n];
		graph[n].statement = TranslateRegion(condition, block, jumpAddr);
	}
	return graph[n].statement;
}

void CGraphTranslator::DumpControlTree()
{
	for (int i = 0; i < graph.GetNodeCount(); ++i)
	{
		auto& block = graph[i];
		COUT << _T("tree node ") << i << _T(" , 前驱 : ");
		DumpNodeSet(block.pred);
		COUT << _T(" 后继 : ");
		DumpNodeSet(block.succ);
		COUT << endl;
	}
}

void CGraphTranslator::DumpCurrentCFG(VertexSet& N)
{
	auto nodes = Nodes(N);
	for (auto i : nodes)
	{
		auto& node = graph[i];
		COUT << _T("tree node ") << i << _T(" , 前驱 : ");
		DumpNodeSet(node.pred);
		COUT << _T(" 后继 : ");
		DumpNodeSet(node.succ);
		COUT << endl;
	}
}
