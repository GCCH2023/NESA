#pragma once
#include "Variable.h"

struct CNode;
struct Type;
struct String;

class Function
{
public:

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
public:
	Type* type;
	Variable* params;
	Variable* variables;
	CNode* body;  // 函数体
	String* name;
};
