#pragma once
//#define DEBUG_GRAPH

//#include <boost/graph/adjacency_list.hpp>
//#include <boost/graph/graph_traits.hpp>
//#include <boost/graph/topological_sort.hpp>
//#include <boost/graph/strong_components.hpp>
//#include <boost/dynamic_bitset/dynamic_bitset.hpp>
//using namespace boost;
//
//class StructuralGraphTranslator
//{
//public:
//	StructuralGraphTranslator(TACFunction* func_, Allocator& allocator_):
//		func(func_),
//		allocator(allocator_),
//		graph(func->GetBasicBlocks().size())
//	{
//	}
//
//private:
//	struct StructuralGraphVertex : ControlTreeNodeEx
//	{
//		dynamic_bitset<> preds;
//		dynamic_bitset<> succs;
//	};
//	// 结构分析图
//	using StructuralGraph = boost::adjacency_list<	vecS, vecS, directedS, StructuralGraphVertex>;
//	using Vertex = boost::graph_traits<StructuralGraph>::vertex_descriptor;
//	using Edge = boost::graph_traits<StructuralGraph>::edge_descriptor;
//
//	// 添加边
//	inline void AddEdge(Vertex from, Vertex to)
//	{
//		add_edge(from, to, graph);
//		graph[from].succs[to] = true;
//		graph[to].preds[from] = true;
//	}
//	// 删除边
//	inline void RemoveEdge(Vertex from, Vertex to)
//	{
//		remove_edge(from, to, graph);
//		graph[from].succs[to] = false;
//		graph[to].preds[from] = false;
//	}
//	// 清空入边
//	inline void ClearInEdges(Vertex vertex)
//	{
//		clear_in_edges(vertex, graph);
//		graph[vertex].preds.reset();
//	}
//	// 清空出边
//	inline void ClearOutEdges(Vertex vertex)
//	{
//		clear_out_edges(vertex, graph);
//		graph[vertex].succs.reset();
//	}
//	// 构建控制流图
//	void BuildCFG()
//	{
//		auto& blocks = func->GetBasicBlocks();
//		// 给基本块编号
//		for (size_t i = 0; i < blocks.size(); ++i)
//		{
//			blocks[i]->tag = (void*)i;
//			graph[i].preds.resize(blocks.size());
//			graph[i].succs.resize(blocks.size());
//		}
//		// 添加边
//		for (auto block : blocks)
//		{
//			for (auto succ : block->nexts)
//			{
//				AddEdge((Vertex)block->tag, (Vertex)succ->tag);
//			}
//		}
//		// 标记所有需要处理的节点
//		reduced.resize(blocks.size());
//	}
//
//	CNode* ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr)
//	{
//		// 条件跳转指令必定是基本块结束指令
//		condition = allocator.New<CNode>(kind, GetExpression(tac->x), GetExpression(tac->y));
//		jumpAddr = tac->z.GetValue();
//		return condition;
//	}
//
//
//	CNodeKind TranslateOperator(TACOperator op)
//	{
//		switch (op)
//		{
//		case	TACOperator::BOR: return CNodeKind::EXPR_BOR;
//		case	TACOperator::BAND: return CNodeKind::EXPR_BAND;
//		case	TACOperator::ASSIGN: return CNodeKind::EXPR_ASSIGN;
//		case	TACOperator::ADD: return CNodeKind::EXPR_ADD;
//		case	TACOperator::SUB: return CNodeKind::EXPR_SUB;
//		case	TACOperator::XOR: return CNodeKind::EXPR_XOR;
//		case TACOperator::SHL: return CNodeKind::EXPR_SHIFT_LEFT;
//		case TACOperator::SHR: return CNodeKind::EXPR_SHIFT_RIGHT;
//		default:
//		{
//			TCHAR buffer[64];
//			_stprintf_s(buffer, _T("三地址码操作码转C表达式：未实现的三地址码操作码 %s"), ToString(op));
//			throw Exception(buffer);
//		}
//		}
//	}
//
//	CNode* TranslateCall(TAC* call, CNode* params)
//	{
//		String* name = nullptr;
//		if (call->x.IsAddress())  // 直接给出函数地址
//		{
//			Sprintf<> s;
//			s.Format(_T("sub_%04X"), call->x.GetValue());
//			name = GetCDB().AddString(s.ToString());
//		}
//		else if (call->x.IsTemp())  // 函数指针临时变量
//		{
//			name = GetLocalVariableName(call->x.GetValue());
//		}
//		else if (call->x.IsGlobal())  // 函数指针全局变量
//		{
//			uint32_t addr = call->x.GetValue();
//			auto global = GetCDB().GetGlobalVariable(addr);
//			if (!global)
//			{
//				Sprintf<> s;
//				s.Format(_T("获取全局函数指针 %X 失败"), addr);
//				throw Exception(s.ToString());
//			}
//			name = global->name;
//		}
//		else
//		{
//			Sprintf<> s;
//			s.Format(_T("三地址码翻译为C语句：%04X 解析函数名称失败"), call->address);
//			throw Exception(s.ToString());
//		}
//		CNode* expr = allocator.New<CNode>(name, params);
//		// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
//		if (call->z.IsTemp())
//		{
//			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(call->z), expr);
//		}
//		return allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//	}
//
//	// 临时变量必定是两条三地址码连着，所以直接合并成一个表达式
//	CNode* TranslateRegion(CNode*& pCondition, TACBasicBlock* tacBlock, uint32_t& jumpAddr)
//	{
//		CNode* current = nullptr, * head = nullptr, * tail = nullptr;
//		CNode* expr = nullptr;
//		auto& codes = tacBlock->GetCodes();
//		for (size_t i = 0; i < codes.size(); ++i)
//		{
//			auto tac = codes[i];
//			switch (tac->op)
//			{
//			case	TACOperator::BOR:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_BOR, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case	TACOperator::BAND:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case	TACOperator::ASSIGN:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), GetExpression(tac->x));
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case	TACOperator::ADD:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ADD, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case	TACOperator::SUB:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_SUB, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case	TACOperator::XOR:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_XOR, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::SHL:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_SHIFT_LEFT, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::SHR:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::BOOL_GREAT:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_GREAT, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::BOOL_GEQ:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_GREAT_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::BOOL_LESS:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_LESS, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::BOOL_LEQ:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_LESS_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::BOOL_EQ:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::BOOL_NEQ:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_NOT_EQUAL, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::BOOL_BAND:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_NOT_EQUAL, expr, GetExpression(TACOperand(0)));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::BOOL_BIT:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, expr, GetExpression(TACOperand(1)));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::ARRAY_GET:
//			{
//				// 可能是给结构体字段赋值
//				auto x = GetExpression(tac->x);
//				// x 必定是变量
//				assert(x->kind == CNodeKind::EXPR_VARIABLE);
//				auto type = x->variable->type;
//				if (type->GetKind() == TypeKind::Struct)
//				{
//					// y 必定是整数
//					assert(tac->y.IsInterger());
//					// 根据偏移量查找字段
//					auto field = type->GetField(tac->y.GetValue());
//					auto fieldNode = allocator.New<CNode>(field);
//					expr = allocator.New<CNode>(CNodeKind::EXPR_DOT, x, fieldNode);
//				}
//				//else if (type->GetKind() == TypeKind::Pointer)
//				//{
//				   // // y 必定是整数，此时是用指针的高字节赋值 z = *((char*)&x + y)，因为指针占2个字节
//				   // assert(tac->y.IsInterger());
//				   // int offset = tac->y.GetValue();
//				   // // (1) 取指针的地址 &x
//				   // expr = allocator.New<CNode>(CNodeKind::EXPR_ADDR, x);
//				   // // (2) 强制类型转换为 (char*)&x
//				   // expr = allocator.New<CNode>(TypeManager::pValue, expr);
//				   // // (3) 可选的偏移字节 (char*)&x + offset
//				   // if (offset > 0)
//				   // {
//					  //  CNode* offsetNode = allocator.New<CNode>(offset);
//					  //  expr = allocator.New<CNode>(CNodeKind::EXPR_ADD, expr, offsetNode);
//				   // }
//				   // // 解引用
//				   // expr = allocator.New<CNode>(CNodeKind::EXPR_DEREF, expr);
//				//}
//				else
//				{
//					expr = allocator.New<CNode>(CNodeKind::EXPR_INDEX, x, GetExpression(tac->y));
//				}
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//
//			}
//			case TACOperator::ARRAY_SET:
//			{
//				// 可能是给结构体字段赋值
//				auto x = GetExpression(tac->x);
//				// x 必定是变量
//				assert(x->kind == CNodeKind::EXPR_VARIABLE);
//				auto type = x->variable->type;
//				if (type->GetKind() == TypeKind::Struct)
//				{
//					// y 必定是整数
//					assert(tac->y.IsInterger());
//					// 根据偏移量查找字段
//					auto field = type->GetField(tac->y.GetValue());
//					auto fieldNode = allocator.New<CNode>(field);
//					expr = allocator.New<CNode>(CNodeKind::EXPR_DOT, x, fieldNode);
//				}
//				//else if (type->GetKind() == TypeKind::Pointer)
//				//{
//				   // COUT << tac;
//				   // // y 必定是整数，此时是用指针的高字节赋值 z = *((char*)&x + y)，因为指针占2个字节
//				   // assert(tac->y.IsInterger());
//				   // int offset = tac->y.GetValue();
//				   // // (1) 取指针的地址 &x
//				   // expr = allocator.New<CNode>(CNodeKind::EXPR_ADDR, x);
//				   // // (2) 强制类型转换为 (char*)&x
//				   // expr = allocator.New<CNode>(TypeManager::pValue, expr);
//				   // // (3) 可选的偏移字节 (char*)&x + offset
//				   // if (offset > 0)
//				   // {
//					  //  CNode* offsetNode = allocator.New<CNode>(offset);
//					  //  expr = allocator.New<CNode>(CNodeKind::EXPR_ADD, expr, offsetNode);
//				   // }
//				   // // 解引用
//				   // expr = allocator.New<CNode>(CNodeKind::EXPR_DEREF, expr);
//				//}
//				else
//				{
//					expr = allocator.New<CNode>(CNodeKind::EXPR_INDEX, x, GetExpression(tac->y));
//				}
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, expr, GetExpression(tac->z));
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//			case TACOperator::ADDR:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::DEREF:
//				expr = allocator.New<CNode>(CNodeKind::EXPR_DEREF, GetExpression(tac->x));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			case TACOperator::CAST:
//			{
//				auto result = GetExpression(tac->z);
//				assert(result->kind == CNodeKind::EXPR_VARIABLE);
//				auto type = result->variable->type;  // 要转换到的类型
//				expr = allocator.New<CNode>(type, GetExpression(tac->x));
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, result, expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//			case	TACOperator::ARG:
//			{
//				// 若干个 ARG 后面跟着一个 CALL
//				// 遇到 ARG，则要连着后面的直到 CALL 的三地址码一起翻译
//				CNode* params = nullptr;
//				CNode* paramsTail = nullptr;
//				while (codes[i]->op == TACOperator::ARG)
//				{
//					if (!paramsTail)
//					{
//						paramsTail = params = GetExpression(codes[i]->x);
//					}
//					else
//					{
//						paramsTail->AddNext(GetExpression(codes[i]->x);
//						paramsTail = paramsTail->GetNext();
//					}
//					++i;
//				}
//				if (codes[i]->op != TACOperator::CALL)
//					throw Exception(_T("三地址码翻译为C语句：ARG 后面不是 CALL"));
//				// 最后是 CALL 指令
//				current = TranslateCall(codes[i]);
//				break;
//			}
//			case	TACOperator::CALL:
//			{
//				// 如果有参数，则必是 若干个 ARG 后面跟着一个 CALL
//				// 直接出现 CALL，说明没有参数
//				current = TranslateCall(tac);
//				break;
//			}
//
//			case TACOperator::IFGEQ:  // 跳转指令是基本块的最后一条指令
//				ConditionalJump(pCondition, CNodeKind::EXPR_GREAT_EQUAL, tac, jumpAddr);
//				continue;
//			case TACOperator::IFGREAT:
//				ConditionalJump(pCondition, CNodeKind::EXPR_GREAT, tac, jumpAddr);
//				continue;
//			case TACOperator::IFEQ:
//				ConditionalJump(pCondition, CNodeKind::EXPR_EQUAL, tac, jumpAddr);
//				continue;
//			case TACOperator::IFNEQ:
//				ConditionalJump(pCondition, CNodeKind::EXPR_NOT_EQUAL, tac, jumpAddr);
//				continue;
//			case TACOperator::IFLESS:
//				ConditionalJump(pCondition, CNodeKind::EXPR_LESS, tac, jumpAddr);
//				continue;
//			case TACOperator::IFLEQ:
//				ConditionalJump(pCondition, CNodeKind::EXPR_LESS_EQUAL, tac, jumpAddr);
//				continue;
//			case TACOperator::IFTRUE:
//				pCondition = allocator.New<CNode>(CNodeKind::EXPR_NOT_EQUAL,
//					GetExpression(tac->x), GetExpression(TACOperand(0)));
//				jumpAddr = tac->z.GetValue();
//				continue;
//			case TACOperator::IFFALSE:
//				pCondition = allocator.New<CNode>(CNodeKind::EXPR_EQUAL, GetExpression(tac->x), GetExpression(TACOperand(0)));
//				jumpAddr = tac->z.GetValue();
//				continue;
//			case TACOperator::GOTO:
//			{
//				// 新：当作条件总是真的跳转语句来翻译
//				pCondition = allocator.New<CNode>(1);
//				jumpAddr = tac->z.GetValue();
//				continue;
//
//				// 旧： goto 在控制流图中对应一条边，可能被处理成循环结构，也可能就是对应goto语句，
//				// 还不知道该怎么处理
//				//if (tac->z.GetValue() == tac->address)
//				//{
//				// // 跳转到自己的语句翻译为 while (1);
//				// //expr = allocator.New<CInteger>(1);
//				// //current = allocator.New<CWhileStatement>(expr, noneStatement);
//				// current = noneStatement;
//				// pCondition = allocator.New<CNode>(1);
//				// break;
//				//}
//				//auto label = GetLabelName(tac->z.GetValue());
//				//current = allocator.New<CNode>(CNodeKind::STAT_GOTO, label);
//				break;
//			}
//			case TACOperator::BIT:
//			{
//				// 先这样翻译凑合一下，翻译成表达式语句
//				expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//			case TACOperator::RETURN:
//			{
//				if (tac->x.IsZero())  // 目前只能返回 AXY 对象，所以可以这么判断有没有返回值
//				{
//					current = allocator.New<CNode>(CNodeKind::STAT_RETURN);
//					break;
//				}
//				// 有返回值的情况
//				current = allocator.New<CNode>(CNodeKind::STAT_RETURN, GetExpression(tac->x));
//				break;
//			}
//			case TACOperator::ROR:
//			{
//				// C语言中没有ROR运算符，翻译为函数调用好了
//				// void Ror(int*, int)
//				CNode* params = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
//				params->AddNext(GetExpression(tac->y);
//				current = allocator.New<CNode>(GetCDB().AddString(_T("Ror")), params);
//				break;
//			}
//			case TACOperator::ROL:
//			{
//				// C语言中没有ROL运算符，翻译为函数调用好了
//				// void Rol(int*, int)
//				CNode* params = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
//				params->AddNext(GetExpression(tac->y);
//				current = allocator.New<CNode>(GetCDB().AddString(_T("Rol")), params);
//				break;
//			}
//			case TACOperator::PUSH:
//			{
//				// 还不知道怎么翻译push，先翻译为函数调用吧
//				CNode* params = GetExpression(tac->x);
//				expr = allocator.New<CNode>(GetCDB().AddString(_T("Push")), params);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//			case TACOperator::POP:
//			{
//				// 还不知道怎么翻译pop，先翻译为函数调用吧
//				expr = allocator.New<CNode>(GetCDB().AddString(_T("Pop")), (CNode*)nullptr);
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//			case TACOperator::BOOL_FLAGV:
//			{
//				// 翻译为函数调用
//				CNode* params = GetExpression(tac->x);
//				params->AddNext(GetExpression(tac->y);
//				expr = allocator.New<CNode>(GetCDB().AddString(_T("IsOverflow")), (CNode*)nullptr);
//				expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//
//			case TACOperator::CLI:
//			{
//				// 翻译为函数调用
//				expr = allocator.New<CNode>(GetCDB().AddString(_T("Cli")), (CNode*)nullptr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//			case TACOperator::SEI:
//			{
//				// 翻译为函数调用
//				expr = allocator.New<CNode>(GetCDB().AddString(_T("Sei")), (CNode*)nullptr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//			case TACOperator::CLD:
//			{
//				// 翻译为函数调用
//				expr = allocator.New<CNode>(GetCDB().AddString(_T("Cld")), (CNode*)nullptr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//			case TACOperator::SED:
//			{
//				// 翻译为函数调用
//				expr = allocator.New<CNode>(GetCDB().AddString(_T("Sed")), (CNode*)nullptr);
//				current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
//				break;
//			}
//			//case TACOperator::CLC:
//			//case TACOperator::SEC:
//			//case TACOperator::CLV:
//			// 进位和溢出标志都是和其他指令配合使用的，抽象语法树中不应该出现
//			default:
//			{
//				TCHAR buffer[64];
//				_stprintf_s(buffer, _T("三地址码转C语句：未实现的三地址码 %s"), ToString(tac->op));
//				throw Exception(buffer);
//			}
//			}
//			// 构建语句列表
//			if (!head)
//			{
//				head = tail = current;
//			}
//			else
//			{
//				tail->AddNext(current;
//				tail = current;
//			}
//		}
//		Nes::Address firstAddr = codes.empty() ? tacBlock->GetStartAddress() : codes[0]->address;
//		auto ret = NewStatementList(head, tail);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
//		AddAddressMapStatement(firstAddr, ret);  // 记录下这个基本块对应的地址及语句
//		return ret;
//	}
//
//
//	CNode* CombineListIf(CNode* statement, CNode* condition, CNode* body, CNode* elseBody /*= nullptr*/)
//	{
//		auto ifStat = allocator.New<CNode>(CNodeKind::STAT_IF, condition, body, elseBody);
//		if (!statement)
//			return ifStat;
//		return NewStatementPair(statement, ifStat);
//	}
//
//	CNode* GetNotExpression(CNode* expr)
//	{
//		switch (expr->kind)
//		{
//		case CNodeKind::EXPR_GREAT:
//			expr->kind = CNodeKind::EXPR_LESS_EQUAL;
//			break;
//		case CNodeKind::EXPR_GREAT_EQUAL:
//			expr->kind = CNodeKind::EXPR_LESS;
//			break;
//		case CNodeKind::EXPR_LESS:
//			expr->kind = CNodeKind::EXPR_GREAT_EQUAL;
//			break;
//		case CNodeKind::EXPR_LESS_EQUAL:
//			expr->kind = CNodeKind::EXPR_GREAT;
//			break;
//		case CNodeKind::EXPR_EQUAL:
//			expr->kind = CNodeKind::EXPR_NOT_EQUAL;
//			break;
//		case CNodeKind::EXPR_NOT_EQUAL:
//			expr->kind = CNodeKind::EXPR_EQUAL;
//			break;
//		default:
//			throw Exception(_T("未实现的表达式取反类型"));
//		}
//		return expr;
//	}
//
//
//
//	CNode* NewDoWhile(CNode* condition, CNode* body)
//	{
//		// do ; while (condition) => while (condition) ;
//		// 没有循环体或者条件总是为真，则转换为 while 循环
//		if (body->kind == CNodeKind::STAT_NONE || condition->kind == CNodeKind::EXPR_INTEGER)
//			return allocator.New<CNode>(CNodeKind::STAT_WHILE, condition, body);
//		return allocator.New<CNode>(CNodeKind::STAT_DO_WHILE, condition, body);
//	}
//
//
//	CNode* NewStatementPair(CNode* first, CNode* second)
//	{
//		assert(first->GetNext() == nullptr);
//		// 尝试优化
//		// 在这里优化，可能有一个问题：有的地方可能引用了其中一个指针
//		// 合并后，被丢弃了，引用失效。
//		// 但是这个问题也不算是问题，因为区域归约后，子区域一般不访问了
//		// 发现了新的问题，把标签语句给优化掉了，还是生成语句后再优化好了
//		//if (first->kind == CNodeKind::STAT_LIST)
//		//{
//		//	if (second->kind == CNodeKind::STAT_LIST)
//		//	{
//		//		// 合并到末尾
//		//		first->list.tail->AddNext(second->list.head;
//		//		first->list.tail = second->list.tail;
//		//		return first;
//		//	}
//		//	// 添加到末尾
//		//	first->list.tail->AddNext(second;
//		//	first->list.tail = second;
//		//	return first;
//		//}
//		//else if (second->kind == CNodeKind::STAT_LIST)
//		//{
//		//	// 添加到开头
//		//	first->AddNext(second->list.head;
//		//	second->list.head = first;
//		//	return second;
//		//}
//		first->AddNext(second;
//		return NewStatementList(first, second);
//	}
//
//
//
//
//	// 当要将控制流图中的一个自循环节点归约时
//	// a -> a
//	void OnReduceSelfLoop(Vertex n)
//	{
//		auto node = graph[n);
//		if (node.type != CTNTYPE_LEAF)
//		{
//			if (node.condition == nullptr)
//				throw Exception(_T("非叶子自循环节点异常"));
//			node.statement = NewDoWhile(node.condition, node.statement);
//			return;
//		}
//		CNode* condition = nullptr;
//		auto block = this->GetTACFunction()->GetBasicBlocks()[node->index];
//		uint32_t jumpAddr;
//		auto a = TranslateRegion(condition, block, jumpAddr);
//		node.statement = NewDoWhile(condition, a);
//	}
//
//	
//	void OnReducePoint2Loop(Vertex f, Vertex s)
//	{
//		auto first = graph[f);
//		auto second = graph[s);
//		auto node = first;  // 结果
//		CNode* condition = nullptr;
//		auto blocks = this->GetTACFunction()->GetBasicBlocks();
//		uint32_t jumpAddr;
//		if (first.type == CTNTYPE_LEAF)
//		{
//			first.statement = TranslateRegion(condition, blocks[first->index], jumpAddr);
//			// 跳转边翻译为 goto 语句
//			auto gotoStat = allocator.New<CNode>(CNodeKind::STAT_GOTO, GetLabelName(jumpAddr));
//			first.statement = CombineListIf(first.statement, condition, gotoStat);
//		}
//		else
//		{
//			throw Exception(_T("2点循环的第一个节点不是叶子节点的归约未实现"));
//		}
//		if (second.type == CTNTYPE_LEAF)
//		{
//			second.statement = TranslateRegion(condition, blocks[second->index], jumpAddr);
//		}
//		else
//		{
//			assert(second.statement);
//			assert(second.condition);
//			condition = second.condition;
//		}
//		node.statement = NewStatementPair(first.statement, second.statement);
//		node.statement = NewDoWhile(condition, node.statement);
//	}
//
//	void OnReduceIf(Vertex _if, Vertex then)
//	{
//		auto& cond = graph[_if];
//		auto& body = graph[then];
//		auto& node = cond;  // 结果
//		CNode* condition = nullptr;
//		auto blocks = func->GetBasicBlocks();
//		uint32_t jumpAddr;
//		if (cond.type == CTNTYPE_LEAF)
//		{
//			cond.statement = TranslateRegion(condition, blocks[_if], jumpAddr);
//		}
//		else
//		{
//			// 如果不是叶子节点，则之前的归约必然要保留有条件
//			if (!cond.condition)
//				throw Exception(_T("翻译为 if 语句的过程中缺少 if 语句的条件表达式"));
//			condition = cond.condition;
//		}
//		CNode* ifCond = condition;
//		if (body.type == CTNTYPE_LEAF)
//		{
//			body.statement = TranslateRegion(condition, blocks[then], jumpAddr);
//		}
//		// 在 if 语句之前还有一段代码
//		node.statement = CombineListIf(cond.statement, ifCond, body.statement);
//	}
//
//
//	// a -> b, a -> c, b ->c, b -> d, c -> d 翻译为 if (x || y) { c }
//	// 其中 a 包含 条件 x，b 包含条件 y， c是条件满足时要执行的
//	void OnReduceIfOr(Vertex _if, Vertex then, Vertex _else)
//	{
//		auto& a = graph[_if];
//		auto& b = graph[then];
//		auto& c = graph[_else];
//		auto& node = a;
//		auto blocks = func->GetBasicBlocks();
//		uint32_t jumpAddr;
//		CNode* condition1 = nullptr, * condition2 = nullptr;
//		if (a.type == CTNTYPE_LEAF)
//		{
//			a.statement = TranslateRegion(condition1, blocks[_if], jumpAddr);
//		}
//		else
//		{
//			// 如果不是叶子节点，则之前的归约必然要保留有条件
//			if (!a.condition)
//				throw Exception(_T("翻译为 if - or 语句的过程中缺少 if 语句的第1个条件表达式"));
//			condition1 = a.condition;
//		}
//		// 需要根据跳转地址来判断哪个基本块是 then 部分，哪个是 else 部分
//		if (jumpAddr == blocks[then]->GetStartAddress())
//		{
//			// 这种情况，需要交换 then 和 else 部分
//			std::swap(node._if.then, node._if._else);
//			std::swap(b, c);
//		}
//		if (b.type == CTNTYPE_LEAF)
//		{
//			b.statement = TranslateRegion(condition2, blocks[then], jumpAddr);
//		}
//		else
//		{
//			// 如果不是叶子节点，则之前的归约必然要保留有条件
//			if (!b.condition)
//				throw Exception(_T("翻译为 if 语句的过程中缺少 if 语句的第2个条件表达式"));
//			condition2 = b.condition;
//		}
//		// 用 || 连接 a 和 b 的条件，b的条件要取反，因为b条件满足时跳转到d
//		condition2 = GetNotExpression(condition2);
//		condition1 = allocator.New<CNode>(CNodeKind::EXPR_OR, condition1, condition2);
//		if (c.type == CTNTYPE_LEAF)
//		{
//			c.statement = TranslateRegion(condition2, blocks[c->index], jumpAddr);
//		}
//
//		// 多出的那条边翻译为 goto 语句，多出的边的尾节点只有一个后继，所以不会给condition赋值
//		//auto name = GetLabelName(blocks[c->index].GetStartAddress());
//		//auto label = allocator.New<CLabelStatement>(name.c_str(), c.statement);  // 尾节点的语句替换为标签语句
//		//c.statement = label;
//
//		//auto gotoStat = allocator.New<CGotoStatement>(label);
//		//b.statement = CombineListIf(b.statement, condition, b.statement, c.statement);  // 头节点的末尾加上一个条件跳转语句
//
//		// 在 if 语句之前还有一段代码
//		node.statement = CombineListIf(a.statement, condition1, c.statement);
//	}
//
//
//
//	Vertex CReduce(Vertex parent, vector<Vertex> children, CtrlTreeNodeType type)
//	{
//		auto& ctNode = graph[parent];
//		switch (type)
//		{
//		case CTNTYPE_LEAF:
//			throw Exception(_T("不能将区域归约为叶子区域"));
//			break;
//		}
//		ctNode.type = type;
//
//		//COUT << "归约 " << ToString(type) << " " << parent << " : ";
//		//for (auto n : children)
//		//	COUT << n << ", ";
//		//COUT << endl;
//		//ctNode->Dump();
//		//COUT << ctNode->statement;
//		//COUT << endl;
//		return parent;
//	}
//
//	void DumpCurrentCFG()
//	{
//		/*for (auto v : make_iterator_range(vertices(graph)))
//		{
//			auto node = graph[v];
//			COUT << _T("tree node ") << v << _T(" , 前驱 : ");
//			auto [i, e] = in_edges(v, graph);
//			if (i == e)
//				COUT << _T("空");
//			else
//			{
//				for (; i != e; ++i)
//				{
//					COUT << *i << _T(", ");
//				}
//			}
//			COUT << _T(" 后继 : ");
//			COUT << endl;
//		}*/
//	}
//	// 归约自循环 a -> a
//	Vertex ReduceRegionSelfLoop(Vertex a)
//	{
//		// r 的前驱是 a 除了 a 之外的前驱
//		// r 的后继是 a 除了 a 之外的后继
//		RemoveEdge(a, a);
//	
//		// 使用 r 代替 a
//		auto& node = graph[a];
//		if (node.type != CTNTYPE_LEAF)
//		{
//			if (node.condition == nullptr)
//				throw Exception(_T("非叶子自循环节点异常"));
//			node.statement = NewDoWhile(node.condition, node.statement);
//			return;
//		}
//		CNode* condition = nullptr;
//		auto block = func->GetBasicBlocks()[a];
//		uint32_t jumpAddr;
//		auto body = TranslateRegion(condition, block, jumpAddr);
//		node.statement = NewDoWhile(condition, body);
//
//		node.type = CTNTYPE_SELF_LOOP;
//		return CReduce(a, { a }, CTNTYPE_SELF_LOOP);
//	}
//	// 归约两个区域构成的连续区域  a -> b
//	Vertex ReduceRegionList(Vertex a, Vertex b)
//	{
//		// r 的前驱是 a 的前驱
//		// r 的后继是 b 的后继
//		RemoveEdge(a, b);
//		for (auto [i, end] = out_edges(b, graph); i != end; ++i)
//		{
//			AddEdge(a, boost::target(*i, graph));
//		}
//		ClearOutEdges(b);
//
//		// 使用 r 代替 a, b
//		reduced[b] = true;
//
//		auto& first = graph[a];
//		auto& second = graph[b];
//		CNode* condition = nullptr;
//		auto blocks = func->GetBasicBlocks();
//		uint32_t jumpAddr;
//		if (first.type == CTNTYPE_LEAF)
//		{
//			first.statement = TranslateRegion(condition, blocks[a], jumpAddr);
//		}
//		if (second.type == CTNTYPE_LEAF)
//		{
//			second.statement = TranslateRegion(condition, blocks[b], jumpAddr);
//		}
//		first.statement = NewStatementPair(first.statement, second.statement);
//		first.condition = condition;
//
//		graph[a].type = CTNTYPE_LIST;
//		return CReduce(a, { a, b }, CTNTYPE_LIST);
//	}
//	// 归约 if else 结构体 a -> b, a -> c, b -> d, c -> d
//	// a 是 if 的条件所在基本块，b 和 c 是满足条件和不满足条件执行的基本块，d 是 if 执行后到达的基本块
//	Vertex ReduceRegionIfElse(Vertex a, Vertex b, Vertex c)
//	{
//		Vertex r = a;
//		// r 的前驱是 a 的前驱
//		// r 的后继是 b 和 c 的后继，且 b 和 c 都只有一个相同的后继
//		auto [s, sEnd] = adjacent_vertices(b, graph);
//		ClearOutEdges(r);
//		AddEdge(r, *s);
//		RemoveEdge(*s, b);
//		RemoveEdge(*s, c);
//
//		// 使用 r 代替 a, b, c
//		reduced[b] = true;
//		reduced[c] = true;
//
//		auto& cond = graph[a];
//		auto& then = graph[b];
//		auto& _else = graph[c];
//		auto& node = cond;  // 结果
//		CNode* condition = nullptr;
//		auto blocks = func->GetBasicBlocks();
//		uint32_t jumpAddr;
//		if (cond.type == CTNTYPE_LEAF)
//		{
//			cond.statement = TranslateRegion(condition, blocks[a], jumpAddr);
//		}
//		else
//		{
//			// 如果不是叶子节点，则之前的归约必然要保留有条件
//			if (!cond.condition)
//				throw Exception(_T("翻译为 if - else 语句的过程中缺少 if 语句的条件表达式"));
//			condition = cond.condition;
//		}
//		CNode* ifCond = condition;
//		// 需要根据跳转地址来判断哪个基本块是 then 部分，哪个是 else 部分
//		if (jumpAddr == blocks[b]->GetStartAddress())
//		{
//			// 这种情况，需要交换 then 和 else 部分
//			std::swap(node._if.then, node._if._else);
//			std::swap(then, _else);
//		}
//		if (then.type == CTNTYPE_LEAF)
//		{
//			then.statement = TranslateRegion(condition, blocks[b], jumpAddr);
//		}
//		if (_else.type == CTNTYPE_LEAF)
//		{
//			_else.statement = TranslateRegion(condition, blocks[c], jumpAddr);
//		}
//		// 在 if 语句之前还有一段代码
//		node.statement = CombineListIf(cond.statement, ifCond, then.statement, _else.statement);
//		
//
//		graph[r].type = CTNTYPE_IF_ELSE;
//		return CReduce(r, { a, b, c }, CTNTYPE_IF_ELSE);
//	}
//	// 归约 if else 结构体 a -> b, a -> c, b -> d, c -> d 的扩展
//	// 多出一条边 b -> c
//	// a 是 if 的条件所在基本块，b 和 c 是满足条件和不满足条件执行的基本块，d 是 if 执行后到达的基本块
//	Vertex ReduceRegionIfOr(Vertex a, Vertex b, Vertex c)
//	{
//		Vertex r = a;
//		// r 的前驱是 a 的前驱
//		// r 的后继是 b 和 c 的后继 d，且 b -> c, b -> d, c -> d
//		ClearOutEdges(r);
//		for (auto [i, end] = out_edges(c, graph); i != end; ++i)
//		{
//			Vertex succ = target(*i, graph);
//			AddEdge(r, succ);
//			RemoveEdge(b, succ);
//			RemoveEdge(c, succ);
//		}
//
//		// 使用 r 代替 a, b, c
//		reduced[b] = true;
//		reduced[c] = true;
//
//		OnReduceIfOr(a, b, c);
//
//		graph[r].type = CTNTYPE_IF_OR;
//		return CReduce(r, { a, b, c }, CTNTYPE_IF_OR);
//	}
//	// 归约 if 结构体 a -> b, a -> c, b -> c
//	// a 是 if 的条件所在基本块，b 满足条件执行的基本块，c 是 if 执行后到达的基本块
//	Vertex ReduceRegionIf(Vertex a, Vertex b)
//	{
//		Vertex r = a;
//		// r 的前驱是 a 的前驱
//		// r 的后继是 a 和 b 的后继，a 只有 b, c 两个后继，b 只有 c 一个后继
//		ClearOutEdges(r);
//		for (auto [i, end] = out_edges(b, graph); i != end; ++i)
//		{
//			Vertex succ = target(*i, graph);
//			AddEdge(r, succ);
//			RemoveEdge(b, succ);
//		}
//
//		// 使用 r 代替 a, b
//		reduced[b] = true;
//
//		OnReduceIf(a, b);
//		graph[r].type = CTNTYPE_IF;
//		return CReduce(r, { a, b }, CTNTYPE_IF);
//	}
//	// a->b, b->a 且 b 只有一个前驱
//	// a 除了 b之外的后继边，翻译为goto语句
//	Vertex ReduceRegionPoint2Loop(Vertex a, Vertex b)
//	{
//		Vertex r = a;
//		// r 的前驱是 a 除了 b 之外的前驱
//		RemoveEdge(b, r);
//		// a 的除 b 之外的后继翻译为 goto 语句
//		for (auto [i, end] = out_edges(a, graph); i != end; ++i)
//		{
//			Vertex s = target(*i, graph);
//			if (s == b)
//				continue;
//			// a goto s 这条边的goto应该是叶子节点到叶子节点的边
//			// COUT << a << " goto " << s << endl;
//			// 移除这条边
//			RemoveEdge(a, s);
//			// GetLeafEdges({ a, s });
//			/*SetSub(graph[s].pred, a);
//			SetUnion(graph[s].pred, r);*/
//		}
//		// r 的后继是 b 除了 a 之外的后继
//		ClearOutEdges(r);
//		for (auto [i, end] = out_edges(b, graph); i != end; ++i)
//		{
//			Vertex s = target(*i, graph);
//			if (s == a)
//				continue;
//			RemoveEdge(b, s);
//			AddEdge(r, s);
//		}
//
//		// 替换 a，b 为 r
//		reduced[b] = true;
//
//		OnReducePoint2Loop(a, b);
//		graph[r].type = CTNTYPE_P2LOOP;
//		return CReduce(r, { a, b }, CTNTYPE_P2LOOP);
//	}
//
//
//	// 分析语法树结构
//	void StructuralAnalysis()
//	{
//		while (true)
//		{
//			for (Vertex n = 0; n < reduced.size(); ++n)
//			{
//				if (reduced[n])
//					continue;
//
//				auto predCount = in_degree(n, graph);
//				auto succCount = out_degree(n, graph);
//				auto [succBegin, succEnd] = adjacent_vertices(n, graph);
//				switch (succCount)
//				{
//				case 1:
//				{
//					Vertex succ = *succBegin;
//					if (predCount == 1)
//					{
//						if (succ == n)
//						{
//							ReduceRegionSelfLoop(n);
//							goto NEXT;
//						}
//						ReduceRegionList(n, succ);
//						// 下一次循环
//						goto NEXT;
//					}
//					break;
//				}
//				case 2:
//				{
//					auto b = *succBegin;
//					auto c = *(succBegin + 1);
//					// a -> b, a -> c, b -> d, c -> d 归约为 if else 结构
//					if (out_degree(b, graph) == out_degree(c, graph) && out_degree(b, graph) == 1 &&
//						in_degree(b, graph) == 1 && in_degree(c, graph) == 1)
//					{
//						ReduceRegionIfElse(n, b, c);
//						goto NEXT;
//					}
//					// a -> b, a -> c, b -> d, c -> d, b -> c 归约为 if (x || y)
//					if ((graph[b].succs & graph[c].succs).any())  // b 和 c 有相同的后继
//					{
//						if (out_degree(b, graph) == 2 && out_degree(c, graph) == 1 && graph[b].succs[c])
//						{
//							// b -> c 的边翻译为 goto
//							ReduceRegionIfOr(n, b, c);
//							goto NEXT;
//						}
//						if (out_degree(c, graph) == 2 && out_degree(b, graph) == 1 && graph[c].succs[b])
//						{
//							ReduceRegionIfOr(n, c, b);
//							// c -> b 的边翻译为 goto
//							goto NEXT;
//						}
//					}
//
//					if (in_degree(b, graph) == 1 && out_degree(b, graph) == 1 && graph[b].succs[c])
//					{
//						ReduceRegionIf(n, b);
//						//DumpCurrentCFG(N);
//						goto NEXT;
//					}
//					if (in_degree(c, graph) == 1 && out_degree(c, graph) == 1 && graph[c].succs[b])
//					{
//						ReduceRegionIf(n, c);
//						//DumpCurrentCFG(N);
//						goto NEXT;
//					}
//					// 检测 if
//
//					break;
//				}
//				}
//				// 循环检测
//				for (auto s = succBegin; s != succEnd; ++s)
//				{
//					if (*s == n)  // 自循环检测
//					{
//						//DumpCurrentCFG(N);
//						ReduceRegionSelfLoop(n);
//						//DumpCurrentCFG(N);
//						goto NEXT;
//					}
//					// 两点循环 a -> b && b -> a 并且 b 只有一个前驱
//					if (graph[s].succs[n] && in_degree(*s, graph) == 1)
//					{
//						ReduceRegionPoint2Loop(n, *s);
//						goto NEXT;
//					}
//				}
//			}
//			break;
//		NEXT:
//			// DumpCurrentCFG(N);
//			;
//		}
//	}
//
//
//	// 翻译函数的三地址码为抽象语法树
//	CNode* TranslateBody()
//	{
//		BuildCFG();
//
//		//VertexSet N;
//		//try
//		//{
//		//	N = this->graph->GetFullSet();
//		//	//DumpCurrentCFG(N);
//		//	N = CAnalysis(N);
//		//}
//		//catch (Exception& e)
//		//{
//		//	COUT << e.Message() << std::endl;
//		//}
//		//if (N.Count() != 1)  // 也可能只有一个基本块
//		//{
//		//	// DumpCurrentCFG(N);
//		//	Sprintf<> s;
//		//	s.Format(_T("翻译 %04X 时，控制树无法归约到单一根节点"), GetTACFunction()->GetStartAddress());
//		//	throw Exception(s.ToString());
//		//}
//		//// 在全部语句都生成后，回填标签语句
//		//PatchLabels();
//
//		//Vertex n = N.ToVector()[0];
//		//if (graph[n].statement == nullptr)
//		//{
//		//	CNode* condition = nullptr;
//		//	uint32_t jumpAddr;
//		//	auto block = GetTACFunction()->GetBasicBlocks()[n];
//		//	graph[n].statement = TranslateRegion(condition, block, jumpAddr);
//		//}
//		//return graph[n].statement;
//	}
//
//	StructuralGraph graph;
//	TACFunction* func;
//	Allocator allocator;
//	dynamic_bitset<> reduced;  // 已经归约的节点，不需要再处理
//};

