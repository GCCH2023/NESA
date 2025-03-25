#pragma once
#include "Variable.h"

struct CNode;
struct Type;
struct String;

class Function
{
public:
	Function();

	inline void SetBody(CNode* body) { this->body = body; }
	inline CNode* GetBody() { return body; }

	inline void SetType(Type* type) { this->type = type; }
	inline Type* GetType() { return type; }

	void AddParameter(Variable* param);
	// 获取局部变量，不存在返回 nullptr
	const Variable* GetParameter(String* name) const;

	// 添加一个局部变量
	void AddVariable(Variable* variable);
	// 获取局部变量
	const Variable* GetVariable(String* name) const;
	// 获取局部变量列表
	const Variable* GetVariableList() const { return variables; }

	// 获取函数的开始地址
	inline CAddress GetAddress() const { return address; }
	// 设置函数的开始地址
	inline void SetAddress(CAddress addr) { address = addr; }

public:
	String* name;
	Type* type;
	CAddress address;  // 函数开始地址
	Variable* params;  // 形参列表
	Variable* variables;  // 局部变量列表
	CNode* body;  // 函数体
};
