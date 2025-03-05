#pragma once
struct String;
struct Type;

using CAddress = uint32_t;

enum class IdentifierKind
{
	None,  // 未知
	Global,  // 全局变量
	Typedef,  // 类型别名
	Tag,  // struct, union, enum
};

struct Variable
{
	String* name;  // 名称
	Type* type;  // 类型
	union
	{
		Variable* next = nullptr;  // 局部变量使用，指向下一个参数
		CAddress address;  // 全局变量使用，对应的内存地址
	};
	// 变量的值初始值由类型决定，比如int，那么指向一个int的值
	// 数组类型，则指向数组的值
	void* initializer = nullptr;

	Variable();
	Variable(String* name, Type* type);
	Variable(const Variable* other);
};