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
// !!! 不要直接设置next字段，这样会导致align不被计算
struct Field
{
	String* name;  // 名称
	Type* type;  // 类型
	size_t align;  // 字段的对齐字节数，默认是等于它的类型的对齐字节数
	Field* next;  // 指向下一个字段
};

// 函数类型的参数类型列表
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
			String* name;  // 名称
			Enumerator* members;
		}e;
		struct RecordField
		{
			String* name;  // 名称
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
	// 不使用这个构成函数，因为附加字段一般需要动态分配，不能用现有的
	//Type(const Type* type);
	// 获取类别
	inline TypeKind GetKind() const { return (TypeKind)(base & 0xFF); }
	// 获取修饰符
	inline TypeQualifier GetQualifier() const { return (TypeQualifier)((base >> 8) & 0xFF); }
	inline bool IsConst() const { return ((base >> 8) & (int)TypeQualifier::Const) != 0; }
	inline bool IsVolatile() const { return ((base >> 8) & (int)TypeQualifier::Volatile) != 0; }

	// 获取字段数量
	size_t GetFieldsCount() const;
	// 添加一个字段到记录类型的末尾
	void AddField(Field* field);
	// 根据偏移量获取字段，找不到返回 nullptr
	const Field* GetField(int offset);

	// 添加一个枚举值
	void AddEnumerator(Enumerator* enumerator);

	// 往函数类型的参数列表中添加一个参数
	void AddParameter(TypeList* param);
};

// 获取类型占用的字节大小
size_t GetTypeBytes(const Type* type);
// 获取类型的对齐字节数
size_t GetTypeAlign(const Type* type);
// 获取类型的字符串表示
StdString ToString(Type* type);


