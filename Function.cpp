#include "stdafx.h"
#include "Function.h"
#include "Type.h"

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
								  throw Exception(_T("����ָ�������δʵ��"));
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

void DumpParameter(Parameter* param)
{
	DumpType(param->type);
	COUT << _T(" ") << param->name;
}

void Function::DumpDeclaration()
{
	if (type->GetKind() != TypeKind::Function)
		throw Exception(_T("�����������ʱ�����������ʹ���"));
	Type* t = type->f.returnType;
	DumpType(t);  // ��������
	COUT << _T(" ") << name << _T("(");
	// ����
	for (auto p = type->f.params; p; p = p->next)
	{
		DumpParameter(p);
		if (p->next)
			COUT << _T(", ");
	}
	COUT << _T(");");
}
