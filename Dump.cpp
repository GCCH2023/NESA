#include "stdafx.h"
#include "Dump.h"
#include "String.h"
#include "Variable.h"
using namespace std;

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
									 DumpCNode(os, obj->s.then, indent + 1);
									 Indent(os, indent);
									 os << _T("} while(") << obj->s.condition << _T(");\n");
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
									 return os << obj->variable->name;
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

OStream& operator<<(OStream& os, const String* str)
{
	os << str->str;
	return os;
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

void DumpType(Type* type)
{
	switch (type->GetKind())
	{
	case TypeKind::Enum: COUT << _T("enum ") << type->e.name; break;
	case TypeKind::Struct: COUT << _T("struct ") << type->su.name; break;
	case TypeKind::Union: COUT << _T("union ") << type->su.name; break;
	case TypeKind::Pointer:
	{
							  Type* base = type->pa.type;
							  if (base->GetKind() == TypeKind::Function)
								  throw Exception(_T("函数指针的声明未实现"));
							  DumpType(base);
							  COUT << _T("*");
							  break;
	}
	case TypeKind::Array:
	{
							Type* base = type->pa.type;
							DumpType(base);
							COUT << _T("[") << type->pa.count << _T("]");
							break;
	}
	default:
		COUT << ToString(type->GetKind());
	}
}

void DumpParameter(Variable* param)
{
	DumpType(param->type);
	COUT << _T(" ") << param->name;
}

void DumpDeclaration(const Variable* variable)
{
	DumpType(variable->type);
	COUT << _T(" ") << variable->name << _T(";");
}


void DumpDeclaration(Function* func)
{
	auto type = func->type;
	if (type->GetKind() != TypeKind::Function)
		throw Exception(_T("输出函数声明时，函数的类型错误"));
	Type* t = type->f.returnType;
	DumpType(t);  // 返回类型
	COUT << _T(" ") << func->name << _T("(");
	// 参数
	for (auto p = func->params; p; p = p->next)
	{
		DumpParameter(p);
		if (p->next)
			COUT << _T(", ");
	}
	COUT << _T(")");
}


void DumpField(Field* field, int indent)
{
	Indent(COUT, indent);
	DumpType(field->type);
	COUT << _T(" ") << field->name << _T(";\n");
}

void DumpDefinition(Type* type)
{
	int indent = 0;
	switch (type->GetKind())
	{
	case TypeKind::Struct:
		COUT << _T("struct ");
		break;
	case TypeKind::Union:
		COUT << _T("struct ");
		break;
	default:
		return;
	}
	COUT << type->su.name << _T("{\n");
	for (Field* field = type->su.fields; field; field = field->next)
	{
		DumpField(field, indent + 1);
	}
	COUT << _T("};\n");
}

void DumpDefinition(Function* func)
{
	DumpDeclaration(func);  // 函数声明
	COUT << _T("\n{\n");
	for (auto v = func->GetVariableList(); v; v = v->next)
	{
		Indent(COUT, 1);
		DumpDeclaration(v);
		COUT << std::endl;
	}
	DumpCNode(COUT, func->GetBody(), 1);
	COUT << _T("}\n");
}
