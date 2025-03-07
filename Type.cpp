#include "stdafx.h"
#include "Type.h"

const TCHAR* ToString(TypeQualifier qulifier)
{
	static const TCHAR* names[] =
	{
		_T("None"), _T("const"), _T("volatile")
	};
	return names[(int)qulifier];
}

const TCHAR* ToString(TypeKind typeKind)
{
	static const TCHAR* names[] =
	{
		_T("void"),
		_T("char"), _T("short"), _T("int"), _T("long"), _T("long long"),
		_T("unsigned char"), _T("unsigned short"), _T("unsigned int"), _T("unsigned long"), _T("unsigned long long"),
		_T("float"), _T("double"), _T("long double"),
		_T("enum"),
		_T("pointer"), _T("array"), _T("union"), _T("struct"), _T("function")
	};
	return names[(int)typeKind];
}

//const TCHAR* ToString(Type* type)
//{
//	if (type->kind < TypeKind::ENUM)
//		return ToString((TypeKind)type->kind);
//
//	switch (type->kind)
//	{
//	case TypeKind::ENUM:
//
//	}
//}

size_t GetTypeBytes(const Type* type)
{
	switch (type->GetKind())
	{
	case TypeKind::Void: return 0;
	case TypeKind::Char: return 1;
	case TypeKind::Short: return 2;
	case TypeKind::Int: return 4;
	case TypeKind::Long: return 4;
	case TypeKind::LongLong: return 8;
	case TypeKind::UChar: return 1;
	case TypeKind::UShort: return 2;
	case TypeKind::UInt: return 4;
	case TypeKind::ULong: return 4;
	case TypeKind::ULongLong: return 8;
	case TypeKind::Float: return 4;
	case TypeKind::Double: return 8;
	case TypeKind::LongDouble: return 8;
	case TypeKind::Enum: return 4;
	case TypeKind::Pointer: return 2;  // 为了适配6502，改为2个字节
	case TypeKind::Array: return GetTypeBytes(type->pa.type) * type->pa.count;
	case TypeKind::Union:
	{
							size_t size = 0;
							for (auto field = type->su.fields; field; field = field->next)
							{
								size_t bytes = GetTypeBytes(field->type);
								if (bytes > size)
									size = bytes;
							}
							return size ? size : 1;  // 最少要有1个字节
	}
	case TypeKind::Struct:
	{
							 size_t size = 0;
							 size_t align = 0;  // 整个结构体的对齐字节数
							 for (auto field = type->su.fields; field; field = field->next)
							 {
								 size = (size + field->align - 1) & ~(field->align - 1);  // 向上取整到字段的对齐字节
								 size += GetTypeBytes(field->type);
								 if (field->align > align)
									 align = field->align;
							 }
							 size = (size + align - 1) & ~(align - 1);  // 向上取整到字段的对齐字节
							 return size ? size : 1;  // 最少要有1个字节
	}
	case TypeKind::Function:
		return 0;  // 函数不能计算大小
	}
	return 0;
}

size_t GetTypeAlign(const Type* type)
{
	switch (type->GetKind())
	{
	case TypeKind::Void: return 0;
	case TypeKind::Char: return 1;
	case TypeKind::Short: return 2;
	case TypeKind::Int: return 4;
	case TypeKind::Long: return 4;
	case TypeKind::LongLong: return 8;
	case TypeKind::UChar: return 1;
	case TypeKind::UShort: return 2;
	case TypeKind::UInt: return 4;
	case TypeKind::ULong: return 4;
	case TypeKind::ULongLong: return 8;
	case TypeKind::Float: return 4;
	case TypeKind::Double: return 8;
	case TypeKind::LongDouble: return 8;
	case TypeKind::Enum: return 4;
	case TypeKind::Pointer: return 4;
	case TypeKind::Array: return GetTypeAlign(type->pa.type);
	case TypeKind::Union:
	case TypeKind::Struct:
	{
							 size_t size = 0;
							 for (auto field = type->su.fields; field; field = field->next)
							 {
								 size_t align = field->align;
								 if (align > size)
									 size = align;
							 }
							 return size;
	}
	case TypeKind::Function:
		return 0;  // 函数没有对齐
	}
	return 0;
}

Type::Type() :
base(0),
pa({0})
{
}


Type::Type(TypeKind kind, TypeQualifier qualifier) :
base(((int)kind & 0xFF) | (((int)qualifier & 0xFF) << 8)),
pa({ 0 })
{

}


//Type::Type(const Type* type):
//base(type->base),
//pa(type->pa)
//{
//
//}

size_t Type::GetFieldsCount() const
{
	assert(GetKind() == TypeKind::Struct || GetKind() == TypeKind::Union);
	size_t count = 0;
	for (const Field* field = su.fields; field; field = field->next)
		++count;
	return count;
}

void Type::AddField(Field* field)
{
	assert(GetKind() == TypeKind::Struct && GetKind() == TypeKind::Union);
	if (field->align == 0)
		field->align = GetTypeAlign(field->type);
	if (!su.fields)
	{
		su.fields = field;
		return;
	}
	Field* tail;
	for (tail = su.fields; tail->next; tail = tail->next)
		;
	tail->next = field;
}

const Field* Type::GetField(int offset)
{
	assert(GetKind() == TypeKind::Struct);
	size_t size = 0;
	for (const Field* field = su.fields; field; field = field->next)
	{
		size = (size + field->align - 1) & ~(field->align - 1);  // 向上取整到字段的对齐字节
		if (size == offset)
			return field;
		if (size > offset)
			return nullptr;

		size += GetTypeBytes(field->type);
	}
	return nullptr;
}

void Type::AddEnumerator(Enumerator* enumerator)
{
	if (GetKind() != TypeKind::Enum)
		return;
	if (e.members == nullptr)
	{
		e.members = enumerator;
		return;
	}
	Enumerator* tail;
	for (tail = e.members; tail->next; tail = tail->next)
		;
	tail->next = enumerator;
}

void Type::AddParameter(TypeList* param)
{
	if (GetKind() != TypeKind::Function)
		return;
	if (f.params == nullptr)
	{
		f.params = param;
		return;
	}
	TypeList* tail;
	for (tail = f.params; tail->next; tail = tail->next)
		;
	tail->next = param;
}

