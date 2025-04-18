#include "stdafx.h"
#include "Dump.h"
#include "Variable.h"
#include "CDataBase.h"
using namespace std;
#include "StringJoiner.h"

// 缩进
OStream& Indent(OStream& os, int indent)
{
	for (int i = 0; i < indent; ++i)
		os << '\t';
	return os;
}

void QualifierToString()
{

}

void DumpType(const Type* type);

// 输出类型列表
void DumpTypeList(const TypeList* typeList)
{
	while (typeList)
	{
		DumpType(typeList->type);
		typeList = typeList->next;
		if (typeList)
			COUT << _T(", ");
	}
}

void DumpTypeQualifier(const Type* type)
{
	auto qualifer = type->GetQualifier();
	if (qualifer != TypeQualifier::None)
		COUT << ToString(qualifer) << _T(" ");
}


void DumpType(const Type* type)
{
	DumpTypeQualifier(type);
	switch (type->GetKind())
	{
	case TypeKind::Enum: COUT << _T("enum ") << type->e.name; break;
	case TypeKind::Struct: COUT << _T("struct ") << type->su.name; break;
	case TypeKind::Union: COUT << _T("union ") << type->su.name; break;
	case TypeKind::Pointer:
	{
							  Type* base = type->pa.type;
							  if (base->GetKind() == TypeKind::Function)
							  {
								  auto funcType = type->pa.type;
								  DumpType(funcType->f.returnType);  // 返回值
								  COUT << _T("(*)(");  // 函数名
								  DumpTypeList(funcType->f.params);
								  COUT << _T(");");
								  break;
							  }
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

// 根据运算符的优先级来输出表达式节点
// 如果该节点的优先级低于父节点优先级，则输出括号
OStream& DumpExpression(OStream& os, CNode* child, CNodeKind parentKind)
{
	if (GetOperatorPriority(child->kind) > GetOperatorPriority(parentKind))
	{
		COUT << _T("(") << child << _T(")");
	}
	else
	{
		COUT << child;
	}
	return os;
}

OStream& DumpCNode(OStream& os, const CNode* obj, int indent)
{
	switch (obj->kind)
	{
	case CNodeKind::STAT_LIST:
	{
								 for (auto n = obj->list.head; n; n = n->GetNext())
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
	case CNodeKind::STAT_FOR:
	{
		Indent(os, indent);
		os << _T("for (") << obj->_for.init << _T(";") << obj->_for.condition << _T(";") << obj->_for.iter << _T(")");
		if (obj->s.then->kind == CNodeKind::STAT_NONE)
			return os << _T(" ;\n");
		os << _T(" {\n");
		DumpCNode(os, obj->_for.body, indent + 1);
		Indent(os, indent);
		os << _T("}\n");
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
	case CNodeKind::EXPR_FIELD:
	{
									 return os << obj->field->name;
	}

	// 双目运算符
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
	{
								DumpExpression(os, obj->e.x, obj->kind);
								os << _T(" ") << ToString(obj->kind) << _T(" ");
								return DumpExpression(os, obj->e.y, obj->kind);
	}
	case CNodeKind::EXPR_ARROW:
	case CNodeKind::EXPR_DOT:
	{
								DumpExpression(os, obj->e.x, obj->kind);
								os << ToString(obj->kind);
								return DumpExpression(os, obj->e.y, obj->kind);
	}

	// 单目运算符
	case CNodeKind::EXPR_NOT:
	case CNodeKind::EXPR_DEREF:
	case CNodeKind::EXPR_ADDR:
	{
								 os << ToString(obj->kind);
								 return DumpExpression(os, obj->e.x, obj->kind);
	}
	
	case CNodeKind::EXPR_INDEX:
	{
								  DumpExpression(os, obj->e.x, obj->kind);
								  return os << _T("[") << obj->e.y << _T("]");
	}
	case CNodeKind::EXPR_CALL:
	{
								 Indent(os, indent);
								 os << obj->call.name << _T("(");
								 if (obj->call.params == nullptr)
									 return os << _T(")");
								 for (CNode* param = obj->call.params; param; param = param->GetNext())
								 {
									 if (param != obj->call.params)
										 os << _T(", ");
									 os << param;
								 }
								 return os << _T(")");
	}
	case CNodeKind::EXPR_CAST:
	{
								 os << _T("(");
								 DumpType(obj->cast.type);
								 return os << _T(")") << obj->cast.expr;
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
								 for (auto n = obj->list.head; n; n = n->GetNext())
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
	case CNodeKind::STAT_RETURN:
	{
								   Indent(os, indent);
								   return os << _T("return\n");
	}
	}
	return os;
}


void DumpParameter(Variable* param)
{
	DumpType(param->type);
	COUT << _T(" ") << param->name;
}

inline StringJoiner& operator<<(StringJoiner& strJoiner, const String* name)
{
	return strJoiner << name->str;
}

// 在已有字符串的基础上格式化指定类型
StringJoiner& operator<<(StringJoiner& strJoiner, const Type* type)
{
	auto qualifer = type->GetQualifier();
	if (qualifer != TypeQualifier::None)
		strJoiner << ToString(qualifer) << _T(" ");

	switch (type->GetKind())
	{
	case TypeKind::Pointer:
		if (IsHigher(type->pa.type, type))
			strJoiner << _T("(*") >> _T(")");
		else
			strJoiner << _T("*");
		strJoiner << type->pa.type;
		break;
	case TypeKind::Array:
		if (IsHigher(type->pa.type, type))
			strJoiner >> _T("(");
		strJoiner >> _T("[");
		// 输出数组元素数量
		if (type->pa.count > 0)
		{
			Sprintf<> s;
			s.Format(_T("%d"), type->pa.count);
			strJoiner >> s.ToString();
		}
		strJoiner >> _T("]");
		if (IsHigher(type->pa.type, type))
			strJoiner >> _T(")");
		strJoiner << type->pa.type;
		break;
	case TypeKind::Function:
		if (IsHigher(type->f.returnType, type))
			strJoiner >> _T("(");
		strJoiner >> _T("(");
		// 输出函数参数类型列表
		{
			auto typeList = type->f.params;
			if (typeList)
			{
				strJoiner << typeList->type;
				typeList = typeList->next;
			}
			while (typeList)
			{
				strJoiner >> _T(", ");
				strJoiner << typeList->type;
				typeList = typeList->next;
			}
		}
		strJoiner >> _T(")");
		if (IsHigher(type->f.returnType, type))
			strJoiner >> _T(")");
		strJoiner << type->pa.type;
		break;
	case TypeKind::Enum: strJoiner << type->e.name << _T("enum "); break;
	case TypeKind::Struct: strJoiner << type->su.name << _T("struct "); break;
	case TypeKind::Union: strJoiner << type->su.name << _T("union "); break;
	default:
		strJoiner << ToString(type->GetKind());
	}
	return strJoiner;
}

void DumpDeclaration(const Variable* variable)
{
	StringJoiner strJoiner;
	strJoiner << variable->name << _T(" ") << variable->type >> _T(";");
	COUT << strJoiner.ToString();
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
	// 局部变量声明
	for (auto v = func->GetVariableList(); v; v = v->next)
	{
		Indent(COUT, 1);
		DumpDeclaration(v);
		COUT << std::endl;
	}
	COUT << endl;
	DumpCNode(COUT, func->GetBody(), 1);
	COUT << _T("}\n");
}

void Dump(CDataBase& cdb)
{
	// 输出标签定义
	for (auto t : cdb.GetTagList())
	{
		DumpDefinition(t);
		COUT << std::endl;
	}
	COUT << std::endl;
	// 输出函数声明
	for (auto f : cdb.GetFunctionList())
	{
		DumpDeclaration(f);
		COUT << _T(";\n");
	}
	COUT << std::endl;
	// 输出全局变量
	for (auto g : cdb.GetGlobalList())
	{
		DumpDeclaration(g);
		COUT << std::endl;
	}
	COUT << std::endl;
	// 输出函数定义
	for (auto f : cdb.GetFunctionList())
	{
		DumpDefinition(f);
		COUT << std::endl;
	}
}

