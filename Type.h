#pragma once
#include <stdint.h>
#include <tchar.h>

struct String;

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
// !!! ��Ҫֱ������next�ֶΣ������ᵼ��align��������
struct Field
{
	String* name;  // ����
	Type* type;  // ����
	size_t align;  // �ֶεĶ����ֽ�����Ĭ���ǵ����������͵Ķ����ֽ���
	Field* next;  // ָ����һ���ֶ�
};

// �������͵Ĳ��������б�
struct TypeList
{
	Type* type;
	TypeList* next;
};

struct Enumerator
{
	String* name;
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
			String* name;  // ����
			Enumerator* members;
		}e;
		struct RecordField
		{
			String* name;  // ����
			Field* fields;
		}su;  // struct, union
		struct FunctionField
		{
			Type* returnType;
			TypeList* params;
		}f;
	};

	Type();
	Type(TypeKind kind, TypeQualifier qualifier = TypeQualifier::None);
	// ��ʹ��������ɺ�������Ϊ�����ֶ�һ����Ҫ��̬���䣬���������е�
	//Type(const Type* type);
	// ��ȡ���
	inline TypeKind GetKind() const { return (TypeKind)(base & 0xFF); }
	// ��ȡ���η�
	inline TypeQualifier GetQualifier() const { return (TypeQualifier)((base >> 8) & 0xFF); }
	inline bool IsConst() const { return ((base >> 8) & (int)TypeQualifier::Const) != 0; }
	inline bool IsVolatile() const { return ((base >> 8) & (int)TypeQualifier::Volatile) != 0; }

	// ��ȡ�ֶ�����
	size_t GetFieldsCount() const;
	// ���һ���ֶε���¼���͵�ĩβ
	void AddField(Field* field);
	// ����ƫ������ȡ�ֶΣ��Ҳ������� nullptr
	const Field* GetField(int offset);

	// ���һ��ö��ֵ
	void AddEnumerator(Enumerator* enumerator);

	// ���������͵Ĳ����б������һ������
	void AddParameter(TypeList* param);
};

// ��ȡ����ռ�õ��ֽڴ�С
size_t GetTypeBytes(const Type* type);
// ��ȡ���͵Ķ����ֽ���
size_t GetTypeAlign(const Type* type);
// ��ȡ���͵��ַ�����ʾ
StdString ToString(Type* type);


