#pragma once
#include "Variable.h"

struct CNode;
struct Type;
struct String;

struct Parameter
{
	String* name;  // 名称
	Type* type;  // 类型
	Parameter* next;  // 指向下一个参数

	Parameter();
	Parameter(String* name, Type* type);
	Parameter(const Parameter* param);
};

class Function
{
public:

	inline void SetBody(CNode* body) { this->body = body; }
	inline CNode* GetBody() { return body; }

	inline void SetType(Type* type) { this->type = type; }
	inline Type* GetType() { return type; }

	void AddParameter(Parameter* param);

	// 添加一个局部变量
	void AddVariable(Variable* variable);
	// 获取局部变量
	const Variable* GetVariable(String* name) const;
	// 获取局部变量列表
	const Variable* GetVariableList() const { return variables; }
public:
	Type* type;
	Parameter* params;
	Variable* variables;
	CNode* body;  // 函数体
	String* name;
};
