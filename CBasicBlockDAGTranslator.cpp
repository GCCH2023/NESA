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

Expression* CBasicBlockDAGTranslator::UnaryExpression(CNodeKind kind, Expression* x)
{
	return GetNode(Expression::Unary(kind, x));
}

Expression* CBasicBlockDAGTranslator::BinaryExpression(CNodeKind kind, Expression* x, Expression* y)
{
	return GetNode(Expression::Binary(kind, x, y));
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

Expression* CBasicBlockDAGTranslator::GetNode(Expression&& node)
{
	auto it = nodeSet.find(&node);
	if (it != nodeSet.end())
		return (Expression*)*it;
	// 创建
	Expression* n = GetNodeFactory().Copy(&node);
	nodeSet.insert(n);
	return n;
}

Statement* CBasicBlockDAGTranslator::GetNode(Statement&& node)
{
	auto it = nodeSet.find(&node);
	if (it != nodeSet.end())
		return (Statement*)*it;
	// 创建
	Statement* n = GetNodeFactory().Copy(&node);
	nodeSet.insert(n);
	return n;
}

void CBasicBlockDAGTranslator::Attach(const TACOperand& var, CNode* node)
{
	varMap[var] = node;
}

Expression* CBasicBlockDAGTranslator::AssignExpression(const TACOperand& z, Expression* x)
{
	Attach(z, x);
	if (IsAxyNvzc(z) || z.IsGlobal())
		Reserve(z, x);
	return nullptr;
}

Expression* CBasicBlockDAGTranslator::ArrayAssign(Expression* z, Expression* x)
{
	auto expr = GetNode(Expression::Binary(CNodeKind::EXPR_ASSIGN, z, x));
	Reserve(expr);
	return nullptr;
}

Expression* CBasicBlockDAGTranslator::CallExpression(String* func, ConstArgList& args, const TACOperand& result)
{
	Expression* expr = GetNodeFactory().Call(func);
	auto& callArgs = expr->GetArguments();
	for (auto arg : args)
	{
		callArgs.push_back(arg);
	}
	// 如果有返回值，那么接收返回值，返回值必定是用临时变量接收
	if (result.IsTemp())
		expr = AssignExpression(result, expr);
	else
		Reserve(expr);
	return expr;
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

Expression* CBasicBlockDAGTranslator::CastExpression(const Type* type, Expression* value)
{
	return GetNode(Expression::Cast(type, value));
}

void CBasicBlockDAGTranslator::ConditionalJump(CNodeKind op, Expression* x, Expression* y, uint32_t jump)
{
	// 条件跳转指令必定是基本块结束指令
	auto condition = BinaryExpression(op, x, y);
	SetJumpCondition(condition);
	SetJumpTarget(jump);
}

void CBasicBlockDAGTranslator::GenerateConditionalJump(const DefinitionVar& definition)
{
	auto cond = GetJumpCondition();
	if (!cond)
		return;

	cond = GenerateExpression(cond, definition);
	SetJumpCondition(cond);
}

void CBasicBlockDAGTranslator::GenerateDAG(const TACBasicBlock* block)
{
	auto& codes = block->GetCodes();
	for (size_t i = 0; i < codes.size(); ++i)
	{
		auto tac = codes[i];
		TranslateTAC(tac, i);
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
			auto varValue = std::get<TACVar>(expr);
			Expression* right = GenerateExpression(varValue.second, definition);				
			Expression* left = GetNodeFactory().Var(GetVariable(varValue.first));
			expression = GetNodeFactory().Assign(left, right);

			auto iter = definition.find(varValue.second);
			if (iter == definition.end())
				definition.insert({ varValue.second, left->GetVariable() });
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

	if (GetReturnStatement())
	{
		auto value = GenerateExpression(GetReturnStatement()->GetReturnValue(), definition);
		auto stat = GetNodeFactory().Return(value);
		list.push_back(stat);
	}
	return GetTranslator()->NewStatementList(list);  // 可能有一个基本块只由一条跳转指令构成，返回空语句
}

Expression* CBasicBlockDAGTranslator::GenerateExpression(CNode* node, const DefinitionVar& definition)
{
	if (node == nullptr)
		return nullptr;

	// 如果要生成初级表达式，则直接返回它们
	if (!node->IsExpression() || !((Expression*)node)->IsPrimary())
	{
		// 已经生成过赋值代码的变量，直接返回它
		auto it = definition.find(node);
		if (it != definition.end())
			return GetNodeFactory().Var(it->second);
	}

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
	case CNodeKind::EXPR_FIELD:
		return  GetNodeFactory().Field(expr->GetField());
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
	{
		auto left = GenerateExpression(expr->GetLeftOperand(), definition);
		auto right = GenerateExpression(expr->GetRightOperand(), definition);
		return  GetNodeFactory().Binary(node->GetKind(), left, right);
	}
	case CNodeKind::EXPR_CALL:
	{
		auto call = GetNodeFactory().Call(expr->GetFunctionName());
		for (auto arg : expr->GetArguments())
		{

			auto argu = GenerateExpression(arg, definition);
			call->GetArguments().push_back(argu);
		}
		return call;
	}

	case CNodeKind::EXPR_NOT:
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
	{
		auto x = GenerateExpression(expr->GetOperand(), definition);
		return  GetNodeFactory().Unary(node->GetKind(), x);
	}
	case CNodeKind::EXPR_CAST:
	{
		auto x = GenerateExpression(expr->GetCastValue(), definition);
		return  GetNodeFactory().Cast(expr->GetCastType(), x);
	}

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
