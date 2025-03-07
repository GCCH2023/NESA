#include "stdafx.h"
#include "CNode.h"

CNode::CNode(CNodeKind kind_, uint32_t address_) :
kind(kind_),
address(address_)
{

}

CNode::CNode(const Variable* variable_) :
kind(CNodeKind::EXPR_VARIABLE),
variable(variable_)
{
}


CNode::CNode(const Field* field_):
kind(CNodeKind::EXPR_FIELD),
field(field_)
{

}

CNode::CNode(int value) :
kind(CNodeKind::EXPR_INTEGER)
{
	i.value = value;
}

CNode::CNode(CNodeKind kind_, CNode* x, CNode* y /*= nullptr*/, CNode* z /*= nullptr*/) :
kind(kind_)
{
	e.x = x;
	e.y = y;
	e.z = z;
}

CNode::CNode(String* name, CNode* params /*= nullptr*/) :
kind(CNodeKind::EXPR_CALL)
{
	f.name = name;
	f.params = params;
}

CNode::CNode(CNodeKind kind_, String* name) :
kind(kind_)
{
	l.name = name;
}

CNode::CNode()
{

}
