#pragma once
struct CNode;
struct Type;

class Function
{
public:

	inline void SetBody(CNode* body) { this->body = body; }
	inline CNode* GetBody() { return body; }

	inline void SetType(Type* type) { this->type = type; }
	inline Type* GetType() { return type; }

	// 输出函数声明
	void DumpDeclaration();
public:
	Type* type;
	CNode* body;  // 函数体
	String name;
};
