#pragma once
#include "Type.h"

class Allocator;

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
	Type* Void;
	Type* Char;
	Type* Short;
	Type* Int;
	Type* Long;
	Type* LongLong;
	Type* UnsignedChar;
	Type* UnsignedShort;
	Type* UnsignedInt;
	Type* UnsignedLong;
	Type* UnsignedLongLong;
	Type* Float;
	Type* Double;
	Type* LongDouble;

	TypeManager(Allocator& allocator);
	// 创建一个数组类型
	Type* NewArray(Type* elementType, size_t count);
	// 创建一个指针类型
	Type* NewPointer(Type* pointerToType);
	// 创建一个结构体
	Type* NewStruct(const TCHAR* name = nullptr, Field* fileds = nullptr);
	// 创建一个联合体
	Type* NewUnion(const TCHAR* name = nullptr, Field* fileds = nullptr);
	// 创建一个枚举
	Type* NewEnum(const TCHAR* name = nullptr, Enumerator* members = nullptr);
	// 创建一个函数，函数类型是可以复用的
	Type* NewFunction(Type* functionType);

protected:
	// 如果存在指定类型，则返回已有的，否则创建新的返回
	Type* GetType(Type* type);
protected:
	Allocator& allocator;
	std::unordered_set<Type*, TypeHash, TypeEqual> types;
};
