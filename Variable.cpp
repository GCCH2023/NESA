#include "stdafx.h"
#include "Variable.h"


Variable::Variable() :
name(nullptr),
type(nullptr)
{

}

Variable::Variable(String* name_, Type* type_) :
name(name_),
type(type_)
{

}

Variable::Variable(const Variable* other) :
name(other->name),
type(other->type)
{

}
