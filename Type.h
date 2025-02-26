#pragma once
#include <stdint.h>
#include <tchar.h>

enum class TypeQualifier
{
	None = 0,
	Const = 1,
	Volatile = 2,
};

const TCHAR* ToString(TypeQualifier qulifier);

enum class TypeKind
{
	Void,
	Char, Short, Int, Long, LongLong,
	UChar, UShort, UInt, ULong, ULongLong,
	Float, Double, LongDouble,
	Enum,
	Pointer, Array, Union, Struct, Function
};

const TCHAR* ToString(TypeKind typeKind);

struct Type;
struct Field
{
	const TCHAR* name;  // ����
	Type* type;  // ����
	size_t align;  // �ֶεĶ����ֽ�����Ĭ���ǵ����������͵Ķ����ֽ���
	Field* next;  // ָ����һ���ֶ�
};

struct Parameter
{
	const TCHAR* name;  // ����
	Type* type;  // ����
	Parameter* next;  // ָ����һ������
};

struct Enumerator
{
	const TCHAR* name;
	int value;
	Enumerator* next;
};


struct Type
{
	uint32_t base;  // ������η�
	union
	{
		struct PointerArrayField
		{
			Type* type;  // ָ��ָ������ͣ�������ڵ�����
			size_t count;  // ����Ԫ������
		}pa;
		struct EnumField
		{
			const TCHAR* name;  // ����
			Enumerator* members;
		}e;
		struct RecordField
		{
			const TCHAR* name;  // ����
			Field* fields;
		}su;  // struct, union
		struct FunctionField
		{
			Type* returnType;
			Parameter* params;
		}f;
	};

	Type();
	Type(TypeKind kind, TypeQualifier qualifier = TypeQualifier::None);
	Type(const Type* type);
	// ��ȡ���
	inline TypeKind GetKind() const { return (TypeKind)(base & 0xFF); }
	// ��ȡ���η�
	inline TypeQualifier GetQualifier() const { return (TypeQualifier)((base >> 8) & 0xFF); }

	// ��ȡ�ֶ�����
	size_t GetFieldsCount() const;
	// ���һ���ֶε���¼���͵�ĩβ
	void AddField(Field* field);

	// ���һ��ö��ֵ
	void AddEnumerator(Enumerator* enumerator);
};

// ��ȡ����ռ�õ��ֽڴ�С
size_t GetTypeBytes(const Type* type);
// ��ȡ���͵Ķ����ֽ���
size_t GetTypeAlign(const Type* type);
// ��ȡ���͵��ַ�����ʾ
//const TCHAR* ToString(Type* type);


