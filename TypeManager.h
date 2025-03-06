#pragma once
#include "Type.h"

class Allocator;
struct String;

struct TypeHash
{
	std::size_t operator()(const Type* type) const;
};

// 自定义比较函数
struct TypeEqual
{
	bool operator()(const Type* type1, const Type* type2) const;
};

// 类型管理器
// 提供基本的类型对象并复用类型对象
class TypeManager
{
public:
	// C 语言基本类型
	static Type* Void;
	static Type* Char;
	static Type* Short;
	static Type* Int;
	static Type* Long;
	static Type* LongLong;
	static Type* UnsignedChar;
	static Type* UnsignedShort;
	static Type* UnsignedInt;
	static Type* UnsignedLong;
	static Type* UnsignedLongLong;
	static Type* Float;
	static Type* Double;
	static Type* LongDouble;

	// FC 专用类型
	static Type* Value;  // 值的类型，一个字节
	static Type* pValue;  // 指向值的指针，占2个字节，用于变址寻址
	static Type* ppValue;  // 指向值的2重指针，占2个字节，用于间接变址寻址

private:
	friend TypeManager& GetTypeManager();
	TypeManager(Allocator& allocator);

public:
	// 获取类型管理器使用的分配器
	inline Allocator& GetAllocator() { return allocator; }

	// 创建一个数组类型
	Type* NewArray(Type* elementType, size_t count);
	// 创建一个指针类型
	Type* NewPointer(Type* pointerToType);
	// 创建一个结构体
	Type* NewStruct(String* name = nullptr, Field* fileds = nullptr);
	// 创建一个联合体
	Type* NewUnion(String* name = nullptr, Field* fileds = nullptr);
	// 创建一个枚举
	Type* NewEnum(String* name = nullptr, Enumerator* members = nullptr);
	// 创建一个函数，函数类型是可以复用的
	Type* NewFunction(Type* functionType);

protected:
	// 如果存在指定类型，则返回已有的，否则创建新的返回
	Type* GetType(Type* type);
protected:
	Allocator& allocator;
	std::unordered_set<Type*, TypeHash, TypeEqual> types;
};

// 获取类型管理器的全局实例
TypeManager& GetTypeManager();