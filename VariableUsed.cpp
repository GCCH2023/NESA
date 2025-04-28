#include "stdafx.h"
#include "VariableUsed.h"
#include "CNode.h"

bool VariableUsed::BeforeVisit(CNode* root)
{
	used = false;
	return true;
}

void VariableUsed::OnVisit(Statement* node)
{
	if (used)
		return;

	VisitChildren(node);
}

void VariableUsed::OnVisit(Expression* node)
{
	if (used)
		return;

	if (node->IsVariable())
	{
		used = node->GetVariable()->name == variable->name;
		return;
	}

	VisitChildren(node);
}
