#include "stdafx.h"
#include "CNode.h"

CNode::CNode(CNodeKind kind_, uint32_t address_) :
kind(kind_),
address(address_)
{

}

CNode::CNode(CStr name, VariableKind varKind) :
kind(CNodeKind::EXPR_VARIABLE)
{
	v.name = name;
}

CNode::CNode(int value) :
kind(CNodeKind::EXPR_INTEGER)
{
	i.value = value;
}

CNode::CNode(CNodeKind kind_, CNode* x, CNode* y /*= nullptr*/, CNode* z /*= nullptr*/) :
kind(kind_)
{
	e.x = x;
	e.y = y;
	e.z = z;
}

CNode::CNode(CStr name, CNode* params /*= nullptr*/) :
kind(CNodeKind::STAT_CALL)
{
	f.name = name;
	f.params = params;
}

CNode::CNode(CNodeKind kind_, CStr name) :
kind(kind_)
{
	l.name = name;
}

CNode::CNode()
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
								 for (auto n = obj->list.head; n; n = n->next)
									 DumpCNode(os, n, indent);
								 return os;
	}
	case CNodeKind::STAT_EXPR:
	{
								 Indent(os, indent);
								 return os << obj->e.x << _T(";\n");
	}
	case CNodeKind::STAT_WHILE:
	{
								  Indent(os, indent);
								  os << _T("while (") << obj->s.condition << _T(")");
								  if (obj->s.then->kind == CNodeKind::STAT_NONE)
									  return os << _T(" ;\n");
								  os << _T(" {\n");
								  DumpCNode(os, obj->s.then, indent + 1);
								  Indent(os, indent);
								  os << _T("}\n");
								  return os;
	}
	case CNodeKind::STAT_DO_WHILE:
	{
									 Indent(os, indent);
									 os << _T("do {\n");
									 DumpCNode(os, obj->e.y, indent + 1);
									 Indent(os, indent);
									 os << _T("} while(") << obj->e.x << _T(");\n");
									 return os;
	}
	case CNodeKind::STAT_IF:
	{
							   Indent(os, indent);
							   os << _T("if (") << obj->s.condition << _T(")");
							   if (obj->s.then->kind == CNodeKind::STAT_NONE)
								   return os << _T(" ;\n");
							   os << _T(" {\n");
							   DumpCNode(os, obj->s.then, indent + 1);
							   Indent(os, indent);
							   os << _T("}\n");
							   if (obj->s._else)
							   {
								   Indent(os, indent);
								   os << _T("else {\n");
								   DumpCNode(os, obj->s._else, indent + 1);
								   Indent(os, indent);
								   os << _T("}\n");
							   }
							   return os;
	}
	case CNodeKind::STAT_GOTO:
	{
								 Indent(os, indent);
								 os << _T("goto ") << obj->l.name << _T(";\n");
								 return os;
	}
	case CNodeKind::STAT_LABEL:
	{
								  Indent(os, indent);
								  os << obj->l.name << _T(":");
								  os << _T("\n");
								  DumpCNode(os, obj->l.body, indent);
								  return os;
	}
	case CNodeKind::STAT_NONE:
	{
								 Indent(os, indent);
								 return os << _T(";\n");
	}
	case CNodeKind::STAT_CALL:
	{
								 Indent(os, indent);
								 os << obj->f.name << _T("(");
								 if (obj->f.params == nullptr)
									 return os << _T(");\n");
								 for (CNode* param = obj->f.params; param; param = param->next)
								 {
									 if (param != obj->f.params)
										 os << _T(", ");
									 os << param;
								 }
								 return os << _T(");\n");
	}
	case CNodeKind::STAT_RETURN:
	{
								   Indent(os, indent);
								   if (obj->e.x)
									   return os << _T("return ") << obj->e.x << _T(";\n");
								   else
									   return os << _T("return;\n");
	}
	case CNodeKind::EXPR_INTEGER:
	{
									return os << obj->i.value;
	}
	case CNodeKind::EXPR_VARIABLE:
	{
									 return os << obj->v.name;
	}
	case CNodeKind::EXPR_BOR:
	{
								return os << obj->e.x << _T(" | ") << obj->e.y;
	}
	case CNodeKind::EXPR_BAND:
	{
								 return os << obj->e.x << _T(" & ") << obj->e.y;
	}
	case CNodeKind::EXPR_XOR:
	{
								return os << obj->e.x << _T(" ^ ") << obj->e.y;
	}
	case CNodeKind::EXPR_SHIFT_LEFT:
	{
									   return os << obj->e.x << _T(" << ") << obj->e.y;
	}
	case CNodeKind::EXPR_SHIFT_RIGHT:
	{
										return os << obj->e.x << _T(" >> ") << obj->e.y;
	}
	case CNodeKind::EXPR_ADD:
	{
								return os << obj->e.x << _T(" + ") << obj->e.y;
	}
	case CNodeKind::EXPR_SUB:
	{
								return os << obj->e.x << _T(" - ") << obj->e.y;
	}
	case CNodeKind::EXPR_AND:
	{
								return os << obj->e.x << _T(" || ") << obj->e.y;
	}
	case CNodeKind::EXPR_OR:
	{
							   return os << obj->e.x << _T(" || ") << obj->e.y;
	}
	case CNodeKind::EXPR_NOT:
	{
								return os << _T("!") << obj->e.y;
	}
	case CNodeKind::EXPR_ASSIGN:
	{
								   return os << obj->e.x << _T(" = ") << obj->e.y;
	}
	case CNodeKind::EXPR_REF:
	{
								return os << _T("*") << obj->e.x;
	}
	case CNodeKind::EXPR_ADDR:
	{
								 return os << _T("&") << obj->e.x;
	}

	case CNodeKind::EXPR_GREAT:
	{
								  return os << obj->e.x << _T(" > ") << obj->e.y;
	}
	case CNodeKind::EXPR_GREAT_EQUAL:
	{
										return os << obj->e.x << _T(" >= ") << obj->e.y;
	}
	case CNodeKind::EXPR_EQUAL:
	{
								  return os << obj->e.x << _T(" == ") << obj->e.y;
	}
	case CNodeKind::EXPR_NOT_EQUAL:
	{
									  return os << obj->e.x << _T(" != ") << obj->e.y;
	}
	case CNodeKind::EXPR_LESS:
	{
								 return os << obj->e.x << _T(" < ") << obj->e.y;
	}
	case CNodeKind::EXPR_LESS_EQUAL:
	{
									   return os << obj->e.x << _T(" <= ") << obj->e.y;
	}
	case CNodeKind::EXPR_INDEX:
	{
								  return os << obj->e.x << _T("[") << obj->e.y << _T("]");
	}
	default:
		throw Exception(_T("输出节点字符串: 未实现的节点类型"));
	}
}

OStream& operator<<(OStream& os, const CNode* obj)
{
	return DumpCNode(os, obj, 0);
}

OStream& DumpCNodeStructures(OStream& os, const CNode* obj, int indent)
{
	switch (obj->kind)
	{
	case CNodeKind::STAT_LIST:
	{
								 Indent(os, indent);
								 os << _T("list:\n");
								 Indent(os, indent);
								 os << _T("{\n");
								 for (auto n = obj->list.head; n; n = n->next)
									 DumpCNodeStructures(os, n, indent + 1);
								 Indent(os, indent);
								 os << _T("}\n");
								 return os;
	}
	case CNodeKind::STAT_EXPR:
	{
								 Indent(os, indent);
								 return os << _T("expr\n");
	}
	case CNodeKind::STAT_WHILE:
	{
								  Indent(os, indent);
								  os << _T("while:\n");
								  DumpCNodeStructures(os, obj->s.then, indent + 1);
								  return os;
	}
	case CNodeKind::STAT_DO_WHILE:
	{
									 Indent(os, indent);
									 os << _T("do while:\n");
									 DumpCNodeStructures(os, obj->e.y, indent + 1);
									 return os;
	}
	case CNodeKind::STAT_IF:
	{
							   Indent(os, indent);
							   os << _T("if:\n");
							   DumpCNodeStructures(os, obj->s.then, indent + 1);
							   if (obj->s._else)
							   {
								   Indent(os, indent);
								   os << _T("else:\n");
								   DumpCNodeStructures(os, obj->s._else, indent + 1);
							   }
							   return os;
	}
	case CNodeKind::STAT_GOTO:
	{
								 Indent(os, indent);
								 os << _T("goto\n");
								 return os;
	}
	case CNodeKind::STAT_LABEL:
	{
								  Indent(os, indent);
								  os << _T("label:\n");
								  DumpCNodeStructures(os, obj->l.body, indent);
								  return os;
	}
	case CNodeKind::STAT_NONE:
	{
								 Indent(os, indent);
								 return os << _T("null\n");
	}
	case CNodeKind::STAT_CALL:
	{
								 Indent(os, indent);
								 return os << _T("call\n");
	}
	case CNodeKind::STAT_RETURN:
	{
								   Indent(os, indent);
								   return os << _T("return\n");
	}
	}
	return os;
}
