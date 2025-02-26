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
	const TCHAR* name;  // 名称
	Type* type;  // 类型
	size_t align;  // 字段的对齐字节数，默认是等于它的类型的对齐字节数
	Field* next;  // 指向下一个字段
};

struct Parameter
{
	const TCHAR* name;  // 名称
	Type* type;  // 类型
	Parameter* next;  // 指向下一个参数
};

struct Enumerator
{
	const TCHAR* name;
	int value;
	Enumerator* next;
};


struct Type
{
	uint32_t base;  // 类别，修饰符
	union
	{
		struct PointerArrayField
		{
			Type* type;  // 指针指向的类型，数组基于的类型
			size_t count;  // 数组元素数量
		}pa;
		struct EnumField
		{
			const TCHAR* name;  // 名称
			Enumerator* members;
		}e;
		struct RecordField
		{
			const TCHAR* name;  // 名称
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
	// 获取类别
	inline TypeKind GetKind() const { return (TypeKind)(base & 0xFF); }
	// 获取修饰符
	inline TypeQualifier GetQualifier() const { return (TypeQualifier)((base >> 8) & 0xFF); }

	// 获取字段数量
	size_t GetFieldsCount() const;
	// 添加一个字段到记录类型的末尾
	void AddField(Field* field);

	// 添加一个枚举值
	void AddEnumerator(Enumerator* enumerator);
};

// 获取类型占用的字节大小
size_t GetTypeBytes(const Type* type);
// 获取类型的对齐字节数
size_t GetTypeAlign(const Type* type);
// 获取类型的字符串表示
//const TCHAR* ToString(Type* type);


