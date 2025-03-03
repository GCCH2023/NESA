#pragma once
struct String;
struct Type;

using CAddress = uint32_t;

struct Variable
{
	String* name;  // 名称
	Type* type;  // 类型
	union
	{
		Variable* next;  // 局部变量使用，指向下一个参数
		CAddress address;  // 全局变量使用，对应的内存地址
	};

	Variable();
	Variable(String* name, Type* type);
	Variable(const Variable* other);
};
