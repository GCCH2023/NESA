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
	case CNodeKind::STAT_EMPTY:
		break;
	case CNodeKind::EXPR_CALL:
		hash ^= (size_t)node->call.name;
		if (node->call.args)
		{
			for (auto n : CListNode(node->call.args))
				hash ^= (size_t)n;
		}
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
	case CNodeKind::EXPR_FIELD:
		hash ^= (size_t)node->field->name;
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
	case CNodeKind::EXPR_DOT:
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
	case CNodeKind::STAT_EMPTY:
		return true;
	case CNodeKind::EXPR_CALL:
		if (node1->call.name != node2->call.name)
			return false;
		if (node1->call.args && node2->call.args)
		{
			auto p = node1->call.args->list.head;
			auto q = node2->call.args->list.head;
			while (p++ == q++)
				;
			return p == q;
		}
		return false;
	case CNodeKind::EXPR_INTEGER:
		return node1->i.value == node2->i.value;
	case CNodeKind::EXPR_VARIABLE:
		return node1->variable->name == node2->variable->name;
	case CNodeKind::EXPR_FIELD:
		return node1->field->name == node2->field->name;
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
	case CNodeKind::EXPR_DOT:
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
	CBasicBlockBaseTranslator(translator_),
	condition(nullptr),
	jumpAddr(0)
{
}

CNode* CBasicBlockDAGTranslator::TranslateTAC(const TAC* tac, size_t& index)
{
	CNode expr;
	CNode* current = nullptr;
	switch (tac->op)
	{
	default:
		return CBasicBlockBaseTranslator::TranslateTAC(tac, index);
	case TACOperator::BOOL_BAND:
		expr.Expr(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
		expr.Expr(CNodeKind::EXPR_NOT_EQUAL, GetNode(&expr), GetExpression(TACOperand(0)));
		current = GetNode(&expr);
		Attach(tac->z, current);
		return current;
	case TACOperator::BOOL_BIT:
		expr.Expr(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y));
		expr.Expr(CNodeKind::EXPR_BAND, GetNode(&expr), GetExpression(TACOperand(1)));
		current = GetNode(&expr);
		Attach(tac->z, current);
		return current;
	case TACOperator::CAST:
	{
		auto result = GetExpression(tac->z);
		assert(result->kind == CNodeKind::EXPR_VARIABLE);
		auto type = result->variable->type;  // 要转换到的类型

		expr.Cast(type, GetExpression(tac->x));
		auto ret = GetNode(&expr);
		Attach(tac->z, ret);
		return ret;
	}

	case TACOperator::IFGEQ:  // 跳转指令是基本块的最后一条指令
	case TACOperator::IFGREAT:
	case TACOperator::IFEQ:
	case TACOperator::IFNEQ:
	case TACOperator::IFLESS:
	case TACOperator::IFLEQ:
	case TACOperator::IFTRUE:
	case TACOperator::IFFALSE:
		ConditionalJump(tac);
		return nullptr;
	case TACOperator::GOTO:
	{
		// 新：当作条件总是真的跳转语句来翻译
		condition = GetNodeFactory().Integer(1);
		jumpAddr = tac->z.GetValue();
		return nullptr;

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
		expr.Expr(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
		GetNode(&expr);
		break;
	}
	case TACOperator::RETURN:
	{
		if (tac->x.IsZero())  // 目前只能返回 AXY 对象，所以可以这么判断有没有返回值
		{
			expr.Return();
			GetNode(&expr);
			break;
		}
		// 有返回值的情况
		expr.Return(GetExpression(tac->x));
		auto ret = GetNode(&expr);
		Reserve(ret);
		break;
	}
	// 进位和溢出标志都是和其他指令配合使用的，抽象语法树中不应该出现
	}
	return nullptr;
}

CNode* CBasicBlockDAGTranslator::GetExpression(const TACOperand& operand)
{
	auto it = varMap.find(operand);
	if (it != varMap.end())
		return it->second;

	CNode node;
	GetTranslator()->GetExpression(node, operand);
	return GetNode(&node);
}


CNode* CBasicBlockDAGTranslator::TranslateCall(const TAC* call, CNode* params)
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
		name = GetTranslator()->GetLocalVariableName(call->x.GetValue());
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
	CNode expr;
	expr.Call(name, params);
	// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
	auto ret = GetNode(&expr);
	if (call->z.IsTemp())
	{
		Attach(call->z, ret);
		Reserve(call->z);
	}
	else
	{
		Reserve(ret);
	}
	return ret;
}

void CBasicBlockDAGTranslator::ConditionalJump(const TAC* tac)
{
	// 记录下跳转指令
	jumpTAC = tac;
}

CNode* CBasicBlockDAGTranslator::GetNode(CNode* node)
{
	auto it = nodeSet.find(node);
	if (it != nodeSet.end())
		return *it;
	// 创建
	CNode* n = GetNodeFactory().Copy(*node);
	nodeSet.insert(n);
	return n;
}

void CBasicBlockDAGTranslator::Attach(const TACOperand& var, CNode* node)
{
	varMap[var] = node;
}

CNode* CBasicBlockDAGTranslator::BinAssignStatement(CNodeKind kind, const TAC* tac)
{
	CNode node;
	node.Expr(kind, GetExpression(tac->x), GetExpression(tac->y));
	CNode* expr = GetNode(&node);
	Attach(tac->z, expr);
	return expr;
}

CNode* CBasicBlockDAGTranslator::AssignStatement(const TACOperand& z, CNode* x)
{
	Attach(z, x);
	return x;
}

CNode* CBasicBlockDAGTranslator::FieldExpression(CNode* obj, const Field* field)
{
	CNode expr;
	expr.Field(field);
	expr.Expr(CNodeKind::EXPR_DOT, obj, GetNode(&expr));
	return GetNode(&expr);
}

CNode* CBasicBlockDAGTranslator::IndexExpression(CNode* array, CNode* index)
{
	CNode expr;
	expr.Expr(CNodeKind::EXPR_INDEX, array, index);
	return GetNode(&expr);
}

CNode* CBasicBlockDAGTranslator::ArrayAssign(CNode* z, CNode* x)
{
	CNode expr;
	expr.Expr(CNodeKind::EXPR_ASSIGN, z, x);
	return GetNode(&expr);
}

CNode* CBasicBlockDAGTranslator::UnaryAssignStatement(CNodeKind kind, const TAC* tac)
{
	CNode node;
	node.Expr(kind, GetExpression(tac->x));
	auto expr = GetNode(&node);
	Attach(tac->z, expr);
	return expr;
}

void CBasicBlockDAGTranslator::GenerateConditionalJump(const DefinitionVar& definition)
{
	if (!jumpTAC)
		return;

	CNodeKind op;
	switch (jumpTAC->op)
	{
	case TACOperator::IFGEQ:  // 跳转指令是基本块的最后一条指令
		op = CNodeKind::EXPR_GREAT_EQUAL;
		break;
	case TACOperator::IFGREAT:
		op = CNodeKind::EXPR_GREAT;
		break;
	case TACOperator::IFEQ:
		op = CNodeKind::EXPR_EQUAL;
		break;
	case TACOperator::IFNEQ:
		op = CNodeKind::EXPR_NOT_EQUAL;
		break;
	case TACOperator::IFLESS:
		op = CNodeKind::EXPR_LESS;
		break;
	case TACOperator::IFLEQ:
		op = CNodeKind::EXPR_LESS_EQUAL;
		break;
	case TACOperator::IFTRUE:
		op = CNodeKind::EXPR_NOT_EQUAL;
		break;
	case TACOperator::IFFALSE:
		op = CNodeKind::EXPR_EQUAL;
		break;
	default:
		return;
	}
	CNode* x = GetExpression(jumpTAC->x);
	CNode* y = GetExpression(jumpTAC->y);
	x = GenerateExpression(x, definition);
	y = GenerateExpression(y, definition);
	CNode node;
	node.Expr(op, x, y);
	condition = GenerateExpression(&node, definition);
	jumpAddr = jumpTAC->z.GetValue();
}


void CBasicBlockDAGTranslator::MarkReserve(const TAC* tac)
{
	if (tac->op == TACOperator::ARRAY_SET)
		return;

	if (IsAxyNvzc(tac->z))
		Reserve(tac->z);
}

void CBasicBlockDAGTranslator::GenerateDAG(const TACBasicBlock* block)
{
	CNode* current = nullptr;
	auto& codes = block->GetCodes();
	CNode expr;
	for (size_t i = 0; i < codes.size(); ++i)
	{
		auto tac = codes[i];
		TranslateTAC(tac, i);
		MarkReserve(tac);
	}
}

CNode* CBasicBlockDAGTranslator::GenerateCodes()
{
	// 遍历变量，生成它们的赋值语句
	CNode listNode(CNodeKind::STAT_LIST);
	CListNode list(listNode);
	CNode* current;
	std::unordered_map<CNode*, const Variable*>  definition;  // 已经赋值的变量
	for (auto expr : reserved)
	{
		if (expr.index() == 0)  // TACOperand
		{
			// 生成赋值表达式
			auto it = varMap.find(std::get<TACOperand>(expr));
			CNode* right = GenerateExpression(it->second, definition);
			CNode* left = GetNodeFactory().Var(GetVariable(it->first));
			current = GetNodeFactory().Assign(left, right);
			definition[it->second] = left->variable;
		}
		else  // CNode*
		{
			// 生成数组或字段赋值表达式
			current = GenerateExpression(std::get<CNode*>(expr), definition);
		}
		current = GetNodeFactory().ExprStat(current);
		// 构建语句列表
		list.Add(current);
	}
	GenerateConditionalJump(definition);
	return GetTranslator()->NewStatementList(list);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
}

CNode* CBasicBlockDAGTranslator::GenerateExpression(CNode* node, const DefinitionVar& definition)
{
	// 已经生成过赋值代码的变量，直接返回它
	auto it = definition.find(node);
	if (it != definition.end())
		return GetNodeFactory().Var(it->second);

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
		return  GetNodeFactory().Integer(node->i.value);
	case CNodeKind::EXPR_VARIABLE:
		return  GetNodeFactory().Var(node->variable);
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
		return  GetNodeFactory().Expr(node->kind, node->e.x, node->e.y);

	case CNodeKind::EXPR_NOT:
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
		return  GetNodeFactory().Expr(node->kind, node->e.x);

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
CNode* CBasicBlockDAGTranslator::OnTranslate(const TACBasicBlock* block)
{
	GenerateDAG(block);
	auto node = GenerateCodes();
	auto& codes = block->GetCodes();
	Nes::Address firstAddr = codes.empty() ? block->GetStartAddress() : codes[0]->address;
	GetTranslator()->AddAddressMapStatement(firstAddr, node);  // 记录下这个基本块对应的地址及语句
	return node;
}
