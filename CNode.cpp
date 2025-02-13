#include "stdafx.h"
#include "CNode.h"

CNode::CNode(CNodeKind kind_, uint32_t address_) :
kind(kind_),
address(address_)
{

}

CStatement::CStatement(CNodeKind kind, uint32_t address) :
CNode(kind, address)
{

}


CSingleExpression::CSingleExpression(CNodeKind kind, CExpression* operand_) :
CExpression(kind),
operand(operand_)
{

}


CBinaryExpression::CBinaryExpression(CNodeKind kind, CExpression* left_, CExpression* right_) :
CExpression(kind),
left(left_),
right(right_)
{

}

CVariable::CVariable(LPCTSTR name_, VariableKind kind /*= VAR_KIND_GLOBAL*/) :
CExpression(CNodeKind::EXPR_VARIABLE),
name(name_),
varKind(kind)
{

}

CListStatement::CListStatement(CStatement* first_ /*= nullptr*/, CStatement* second_ /*= nullptr*/) :
CStatement(CNodeKind::STAT_LIST),
first(first_),
second(second_)
{

}

CExpressionStatement::CExpressionStatement(CExpression* expression_ /*= nullptr*/) :
CStatement(CNodeKind::STAT_EXPR),
expression(expression_)
{

}

CInteger::CInteger(int value_) :
CExpression(CNodeKind::EXPR_INTEGER),
value(value_)
{

}

// 缩进
OStream& Indent(OStream& os, int indent)
{
	for (int i = 0; i < indent; ++i)
		os << '\t';
	return os;
}

OStream& DumpCNode(OStream& os, const CNode* obj, int indent)
{
	switch (obj->kind)
	{
	case CNodeKind::STAT_LIST:
	{
								 auto list = (const CListStatement*)obj;
								 DumpCNode(os, list->first, indent);
								 DumpCNode(os, list->second, indent);
								 return os;
	}
	case CNodeKind::STAT_EXPR:
	{
								 Indent(os, indent);
								 auto stat = (const CExpressionStatement*)obj;
								 return os << stat->expression << _T(";\n");
	}
	case CNodeKind::STAT_WHILE:
	{
								  Indent(os, indent);
								  auto stat = (const CWhileStatement*)obj;
								  os << _T("while (") << stat->expression << _T(")");
								  if (stat->body->kind == CNodeKind::STAT_NONE)
									  return os << _T(" ;\n");
								  os << _T(" {\n");
								  DumpCNode(os, stat->body, indent + 1);
								  Indent(os, indent);
								  os << _T("}\n");
								  return os;
	}
	case CNodeKind::STAT_DO_WHILE:
	{
									 Indent(os, indent);
									 auto stat = (const CDoWhileStatement*)obj;
									 os << _T("do {\n");
									 DumpCNode(os, stat->body, indent + 1);
									 Indent(os, indent);
									 os << _T("} while(") << stat->expression << _T(");\n");
									 return os;
	}
	case CNodeKind::STAT_IF:
	{
							   Indent(os, indent);
							   auto stat = (const CIfStatement*)obj;
							   os << _T("if (") << stat->condition << _T(")");
							   if (stat->body->kind == CNodeKind::STAT_NONE)
								   return os << _T(" ;\n");
							   os << _T(" {\n");
							   DumpCNode(os, stat->body, indent + 1);
							   Indent(os, indent);
							   os << _T("}\n");
							   return os;
	}
	case CNodeKind::STAT_GOTO:
	{
								 Indent(os, indent);
								 auto stat = (const CGotoStatement*)obj;
								 os << _T("goto ") << stat->label.c_str() << _T(";\n");
								 return os;
	}
	case CNodeKind::STAT_LABEL:
	{
								  Indent(os, indent);
								  auto stat = (const CLabelStatement*)obj;
								  os << stat->name.c_str() << _T(":");
								  os << _T("\n");
								  DumpCNode(os, stat->statement, indent + 1);
								  return os;
	}
	case CNodeKind::STAT_NONE:
	{
								 Indent(os, indent);
								 return os << _T(";\n");
	}
	case CNodeKind::STAT_CALL:
	{
							auto node = (const CCallStatement*)obj;
							os << node->funcName << _T("(");
							if (node->args == nullptr)
								return os << _T(");\n");
							for (int i = 0; node->args[i]; ++i)
							{
								if (i > 0)
									os << _T(", ");
								os << node->args[i];
							}
							return os << _T(");\n");
	}
	case CNodeKind::STAT_RETURN:
	{
								   auto node = (const CReturnStatement*)obj;
								   if (node->value)
									   return os << _T("return ") << node->value << _T(";\n");
								   else
									   return os << _T("return;\n");
	}
	case CNodeKind::EXPR_INTEGER:
	{
									auto node = (const CInteger*)obj;
									return os << node->value;
	}
	case CNodeKind::EXPR_VARIABLE:
	{
									 auto node = (const CVariable*)obj;
									 return os << node->name.c_str();
	}
	case CNodeKind::EXPR_BOR:
	{
						   auto node = (const CBinaryExpression*)obj;
						   return os << node->left << _T(" | ") << node->right;
	}
	case CNodeKind::EXPR_BAND:
	{
							auto node = (const CBinaryExpression*)obj;
							return os << node->left << _T(" & ") << node->right;
	}
	case CNodeKind::EXPR_ADD:
	{
						   auto node = (const CBinaryExpression*)obj;
						   return os << node->left << _T(" + ") << node->right;
	}
	case CNodeKind::EXPR_SUB:
	{
						   auto node = (const CBinaryExpression*)obj;
						   return os << node->left << _T(" - ") << node->right;
	}
	case CNodeKind::EXPR_AND:
	{
								auto node = (const CBinaryExpression*)obj;
								return os << node->left << _T(" || ") << node->right;
	}
	case CNodeKind::EXPR_OR:
	{
								auto node = (const CBinaryExpression*)obj;
								return os << node->left << _T(" || ") << node->right;
	}
	case CNodeKind::EXPR_NOT:
	{
							   auto node = (const CBinaryExpression*)obj;
							   return os <<_T("!") << node->right;
	}
	case CNodeKind::EXPR_ASSIGN:
	{
							  auto node = (const CBinaryExpression*)obj;
							  return os << node->left << _T(" = ") << node->right;
	}
	case CNodeKind::EXPR_REF:
	{
						   auto node = (const CSingleExpression*)obj;
						   return os << _T("*") << node->operand;
	}


	case CNodeKind::EXPR_GREAT:
	{
							 auto node = (const CBinaryExpression*)obj;
							 return os << node->left << _T(" > ") << node->right;
	}
	case CNodeKind::EXPR_GREAT_EQUAL:
	{
								   auto node = (const CBinaryExpression*)obj;
								   return os << node->left << _T(" >= ") << node->right;
	}
	case CNodeKind::EXPR_EQUAL:
	{
							 auto node = (const CBinaryExpression*)obj;
							 return os << node->left << _T(" == ") << node->right;
	}
	case CNodeKind::EXPR_NOT_EQUAL:
	{
								 auto node = (const CBinaryExpression*)obj;
								 return os << node->left << _T(" != ") << node->right;
	}
	case CNodeKind::EXPR_LESS:
	{
							auto node = (const CBinaryExpression*)obj;
							return os << node->left << _T(" < ") << node->right;
	}
	case CNodeKind::EXPR_LESS_EQUAL:
	{
								  auto node = (const CBinaryExpression*)obj;
								  return os << node->left << _T(" <= ") << node->right;
	}
	default:
		throw Exception(_T("输出节点字符串: 未实现的节点类型"));
	}
}

OStream& operator<<(OStream& os, const CNode* obj)
{
	return DumpCNode(os, obj, 0);
}

CExpression::CExpression(CNodeKind kind, uint32_t address /*= 0*/) :
CNode(kind, address)
{

}

CCallStatement::CCallStatement(CVariable* funcName_ /*= nullptr*/) :
CStatement(CNodeKind::STAT_CALL),
funcName(funcName_),
args(nullptr)
{

}

CWhileStatement::CWhileStatement(CExpression* expression_ /*= nullptr*/, CStatement* body_ /*= nullptr*/) :
CStatement(CNodeKind::STAT_WHILE),
expression(expression_),
body(body_)
{

}

CDoWhileStatement::CDoWhileStatement(CExpression* expression_ /*= nullptr*/, CStatement* body_ /*= nullptr*/) :
CStatement(CNodeKind::STAT_DO_WHILE),
expression(expression_),
body(body_)
{

}

CIfStatement::CIfStatement(CExpression* condition_ /*= nullptr*/, CStatement* body_ /*= nullptr*/, CStatement* _else_ /*= nullptr*/) :
CStatement(CNodeKind::STAT_IF),
condition(condition_),
body(body_),
_else(_else_)
{

}

CGotoStatement::CGotoStatement(LPCTSTR label_ /*= nullptr*/):
CStatement(CNodeKind::STAT_GOTO),
label(label_)
{

}

CLabelStatement::CLabelStatement(LPCTSTR name_, CStatement* statement_ /*= nullptr*/) :
CStatement(CNodeKind::STAT_LABEL),
statement(statement_),
name(name_)
{

}

CReturnStatement::CReturnStatement(CExpression* value_ /*= nullptr*/):
CStatement(CNodeKind::STAT_RETURN),
value(value_)
{

}


