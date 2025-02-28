#pragma once
struct CNode;
struct Type;

struct Parameter
{
	const TCHAR* name;  // 名称
	Type* type;  // 类型
	Parameter* next;  // 指向下一个参数

	Parameter();
	Parameter(const TCHAR* name, Type* type);
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
public:
	Type* type;
	Parameter* params;
	CNode* body;  // 函数体
	String name;
};
