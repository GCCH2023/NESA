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
	// ��ȡ�ֲ������������ڷ��� nullptr
	const Variable* GetParameter(String* name) const;

	// ���һ���ֲ�����
	void AddVariable(Variable* variable);
	// ��ȡ�ֲ�����
	const Variable* GetVariable(String* name) const;
	// ��ȡ�ֲ������б�
	const Variable* GetVariableList() const { return variables; }
public:
	Type* type;
	Variable* params;
	Variable* variables;
	CNode* body;  // ������
	String* name;
};
