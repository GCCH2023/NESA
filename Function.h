#pragma once
#include "Variable.h"

struct CNode;
struct Type;
struct String;

struct Parameter
{
	String* name;  // ����
	Type* type;  // ����
	Parameter* next;  // ָ����һ������

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

	// ���һ���ֲ�����
	void AddVariable(Variable* variable);
	// ��ȡ�ֲ�����
	const Variable* GetVariable(String* name) const;
	// ��ȡ�ֲ������б�
	const Variable* GetVariableList() const { return variables; }
public:
	Type* type;
	Parameter* params;
	Variable* variables;
	CNode* body;  // ������
	String* name;
};
