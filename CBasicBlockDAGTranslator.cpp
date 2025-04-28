#include "stdafx.h"
#include "CBasicBlockDAGTranslator.h"
#include "TACFunction.h"
#include "CNode.h"
#include "CDataBase.h"
#include "CTranslator.h"

std::size_t CNodeHash::operator()(const CNode* node) const
{
	size_t hash = (size_t)node->GetKind();
	size_t* p = (size_t*)((uint8_t*)node + sizeof(CNode));
	size_t* end = (size_t*)((uint8_t*)node + sizeof(Statement));
	while (p < end)
	{
		hash ^= *p;
		++p;
	}
	return hash;
}

bool CNodeEqual::operator()(const CNode* node1, const CNode* node2) const
{
	if (node1->GetKind() != node2->GetKind())
		return false;
	size_t* p = (size_t*)((uint8_t*)node1 + sizeof(CNode));
	size_t* q = (size_t*)((uint8_t*)node2 + sizeof(CNode));
	return memcmp(p, q, sizeof(Statement) - sizeof(CNode)) == 0;
}

CBasicBlockDAGTranslator::CBasicBlockDAGTranslator(CTranslator* translator_) :
	CBasicBlockBaseTranslator(translator_)
{
}

Statement* CBasicBlockDAGTranslator::TranslateTAC(const TAC* tac, size_t& index)
{
	Expression* expr;
	CNode* current = nullptr;
	switch (tac->op)
	{
	default:
		return CBasicBlockBaseTranslator::TranslateTAC(tac, index);
	case TACOperator::BOOL_BAND:
		expr = GetNode(Expression::Binary(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y)));
		expr = GetNode(Expression::Binary(CNodeKind::EXPR_NOT_EQUAL, expr, GetExpression(TACOperand(0))));
		Attach(tac->z, expr);
		return nullptr;
	case TACOperator::BOOL_BIT:
		expr = GetNode(Expression::Binary(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y)));
		expr = GetNode(Expression::Binary(CNodeKind::EXPR_BAND, expr, GetExpression(TACOperand(1))));
		Attach(tac->z, expr);
		return nullptr;
	case TACOperator::CAST:
	{
		auto result = GetExpression(tac->z);
		assert(result->GetKind() == CNodeKind::EXPR_VARIABLE);
		auto type = result->GetVariable()->type;  // 要转换到的类型

		expr = GetNode(Expression::Cast(type, GetExpression(tac->x)));
		Attach(tac->z, expr);
		return nullptr;
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
		expr = GetNodeFactory().Integer(1);
		SetJumpCondition(expr);
		SetJumpTarget(tac->z.GetValue());
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
		expr = GetNode(Expression::Binary(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y)));
		Reserve(expr);
		break;
	}
	case TACOperator::RETURN:
	{
		if (tac->x.IsZero())  // 目前只能返回 AXY 对象，所以可以这么判断有没有返回值
		{
			auto stat = GetNode(Statement::Return());
			Reserve(stat);
			return stat;
		}
		// 有返回值的情况
		auto stat = GetNode(Statement::Return(GetExpression(tac->x)));
		Reserve(stat);
		return stat;
	}
	// 进位和溢出标志都是和其他指令配合使用的，抽象语法树中不应该出现
	}
	return nullptr;
}

Expression* CBasicBlockDAGTranslator::GetExpression(const TACOperand& operand)
{
	auto it = varMap.find(operand);
	if (it != varMap.end())
		return (Expression*)it->second;

	Expression node;
	GetTranslator()->GetExpression(node, operand);
	return GetNode(std::move(node));
}


Statement* CBasicBlockDAGTranslator::TranslateCall(const TAC* call, const std::vector<Expression*>& args)
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
	Expression expr = Expression::Call(name);
	auto& callArgs = expr.GetArguments();
	for (auto arg : args)
	{
		callArgs.push_back(arg);
	}
	// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
	auto ret = GetNode(std::move(expr));
	if (call->z.IsTemp())
	{
		Attach(call->z, ret);
		Reserve(call->z);
	}
	else
	{
		Reserve(ret);
	}
	return GetNodeFactory().ExprStat(ret);
}

void CBasicBlockDAGTranslator::ConditionalJump(const TAC* tac)
{
	// 记录下跳转指令
	jumpTAC = tac;
}

Expression* CBasicBlockDAGTranslator::GetNode(Expression&& node)
{
	auto it = nodeSet.find(&node);
	if (it != nodeSet.end())
		return (Expression*)*it;
	// 创建
	Expression* n = GetNodeFactory().Copy(node);
	nodeSet.insert(n);
	return n;
}

Statement* CBasicBlockDAGTranslator::GetNode(Statement&& node)
{
	auto it = nodeSet.find(&node);
	if (it != nodeSet.end())
		return (Statement*)*it;
	// 创建
	Statement* n = GetNodeFactory().Copy(node);
	nodeSet.insert(n);
	return n;
}

void CBasicBlockDAGTranslator::Attach(const TACOperand& var, CNode* node)
{
	varMap[var] = node;
}

Statement* CBasicBlockDAGTranslator::BinAssignStatement(CNodeKind kind, const TAC* tac)
{
	Expression node = Expression::Binary(kind, GetExpression(tac->x), GetExpression(tac->y));
	CNode* expr = GetNode(std::move(node));
	Attach(tac->z, expr);
	return nullptr;
}

Statement* CBasicBlockDAGTranslator::AssignStatement(const TACOperand& z, Expression* x)
{
	Attach(z, x);
	return nullptr;
}

Expression* CBasicBlockDAGTranslator::FieldExpression(Expression* obj, const Field* field)
{
	auto expr = GetNode(Expression::Field(field));
	expr = GetNode(Expression::Binary(CNodeKind::EXPR_DOT, obj, expr));
	return expr;
}

Expression* CBasicBlockDAGTranslator::IndexExpression(Expression* array, Expression* index)
{
	auto expr = GetNode(Expression::Binary(CNodeKind::EXPR_INDEX, array, index));
	return expr;
}

Statement* CBasicBlockDAGTranslator::ArrayAssign(Expression* z, Expression* x)
{
	auto expr = GetNode(Expression::Binary(CNodeKind::EXPR_ASSIGN, z, x));
	return nullptr;
}

Statement* CBasicBlockDAGTranslator::UnaryAssignStatement(CNodeKind kind, const TAC* tac)
{
	auto expr = GetNode(Expression::Unary(kind, GetExpression(tac->x)));
	Attach(tac->z, expr);
	return nullptr;
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
	Expression* x = GetExpression(jumpTAC->x);
	Expression* y = GetExpression(jumpTAC->y);
	x = GenerateExpression(x, definition);
	y = GenerateExpression(y, definition);
	Expression node = Expression::Binary(op, x, y);
	SetJumpCondition(GenerateExpression(&node, definition));
	SetJumpTarget(jumpTAC->z.GetValue());
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
	auto& codes = block->GetCodes();
	for (size_t i = 0; i < codes.size(); ++i)
	{
		auto tac = codes[i];
		TranslateTAC(tac, i);
		MarkReserve(tac);
	}
}

Statement* CBasicBlockDAGTranslator::GenerateCodes()
{
	std::vector<Statement*> list;
	// 遍历变量，生成它们的赋值语句
	Statement* current;
	Expression* expression;
	std::unordered_map<CNode*, const Variable*>  definition;  // 已经赋值的变量
	for (auto expr : reserved)
	{
		if (expr.index() == 0)  // TACOperand
		{
			// 生成赋值表达式
			auto it = varMap.find(std::get<TACOperand>(expr));
			Expression* right = GenerateExpression(it->second, definition);
			Expression* left = GetNodeFactory().Var(GetVariable(it->first));
			expression = GetNodeFactory().Assign(left, right);
			definition[it->second] = left->GetVariable();
		}
		else  // CNode*
		{
			// 生成数组或字段赋值表达式
			expression = GenerateExpression(std::get<CNode*>(expr), definition);
		}
		current = GetNodeFactory().ExprStat(expression);
		// 构建语句列表
		list.push_back(current);
	}
	GenerateConditionalJump(definition);
	return GetTranslator()->NewStatementList(list);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
}

Expression* CBasicBlockDAGTranslator::GenerateExpression(CNode* node, const DefinitionVar& definition)
{
	// 已经生成过赋值代码的变量，直接返回它
	auto it = definition.find(node);
	if (it != definition.end())
		return GetNodeFactory().Var(it->second);

	auto stat = static_cast<Statement*>(node);
	auto expr = static_cast<Expression*>(node);
	switch (node->GetKind())
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
		return  GetNodeFactory().Integer(expr->GetInteger());
	case CNodeKind::EXPR_VARIABLE:
		return  GetNodeFactory().Var(expr->GetVariable());
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
		return  GetNodeFactory().Binary(node->GetKind(), expr->GetLeftOperand(), expr->GetRightOperand());

	case CNodeKind::EXPR_NOT:
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
		return  GetNodeFactory().Unary(node->GetKind(), expr->GetOperand());

	default:
	{
		Sprintf<> s;
		s.Format(_T("DAG 节点转 AST 节点: 未实现的节点类型 % s"), ToString(node->GetKind()));
		throw Exception(s.ToString());
		break;
	}
	}
}

// 临时变量必定是两条三地址码连着，所以直接合并成一个表达式
Statement* CBasicBlockDAGTranslator::OnTranslate(const TACBasicBlock* block)
{
	GenerateDAG(block);
	auto node = GenerateCodes();
	auto& codes = block->GetCodes();
	Nes::Address firstAddr = codes.empty() ? block->GetStartAddress() : codes[0]->address;
	GetTranslator()->AddAddressMapStatement(firstAddr, node);  // 记录下这个基本块对应的地址及语句
	return node;
}
