#include "stdafx.h"
#include "Function.h"
#include "Type.h"

Function::Function():
name(nullptr),
type(nullptr),
address(0),
params(nullptr),
body(nullptr)
{

}

void Function::AddParameter(Variable* param)
{
	if (this->params == nullptr)
	{
		this->params = param;
		return;
	}
	Variable* tail;
	for (tail = this->params; tail->next; tail = tail->next)
		;
	tail->next = param;
}

const Variable* Function::GetParameter(String* name) const
{
	for (auto p = params; p; p = p->next)
	{
		if (p->name == name)
			return p;
	}
	return nullptr;
}

void Function::AddVariable(Variable* variable)
{
	if (!variable)
		return;
	variables.push_back(variable);
}

const Variable* Function::GetVariable(String* name) const
{
	for (const Variable* v : variables)
	{
		if (v->name == name)
			return v;
	}
	return nullptr;
}
