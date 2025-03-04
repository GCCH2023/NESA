#include "stdafx.h"
#include "Function.h"
#include "Type.h"

Parameter::Parameter() :
name(nullptr),
type(nullptr),
next(nullptr)
{

}

Parameter::Parameter(String* name_, Type* type_) :
name(name_),
type(type_),
next(nullptr)
{

}

Parameter::Parameter(const Parameter* param) :
name(param->name),
type(param->type),
next(nullptr)
{

}



void Function::AddParameter(Parameter* param)
{
	if (this->params == nullptr)
	{
		this->params = param;
		return;
	}
	Parameter* tail;
	for (tail = this->params; tail->next; tail = tail->next)
		;
	tail->next = param;
}

void Function::AddVariable(Variable* variable)
{
	if (this->variables == nullptr)
	{
		this->variables = variable;
		return;
	}
	Variable* tail;
	for (tail = this->variables; tail->next; tail = tail->next)
		;
	tail->next = variable;
}

const Variable* Function::GetVariable(String* name) const
{
	for (const Variable* v = this->variables; v; v = v->next)
	{
		if (v->name == name)
			return v;
	}
	return nullptr;
}
