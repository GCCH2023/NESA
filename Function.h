#pragma once
struct CNode;
struct Type;

struct Parameter
{
	const TCHAR* name;  // ����
	Type* type;  // ����
	Parameter* next;  // ָ����һ������

	Parameter();
	Parameter(const TCHAR* name, Type* type);
	Parameter(const Parameter* param);
};

struct Variable
{
	const TCHAR* name;  // ����
	Type* type;  // ����
	Variable* next;  // ָ����һ������

	Variable();
	Variable(const TCHAR* name, Type* type);
	Variable(const Variable* param);
};



class Function
{
public:

	inline void SetBody(CNode* body) { this->body = body; }
	inline CNode* GetBody() { return body; }

	inline void SetType(Type* type) { this->type = type; }
	inline Type* GetType() { return type; }

	void AddParameter(Parameter* param);

	void AddVariable(Variable* variable);

public:
	Type* type;
	Parameter* params;
	Variable* variables;
	CNode* body;  // ������
	String name;
};
