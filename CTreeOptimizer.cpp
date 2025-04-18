#include "stdafx.h"
#include "CTreeOptimizer.h"
#include "CASTListOptimizer.h"
#include "CASTDoWhileOptimier.h"


CTreeOptimizer::CTreeOptimizer()
{
}


CTreeOptimizer::~CTreeOptimizer()
{
}

void CTreeOptimizer::Optimize(CNode* root)
{
	CASTContextTraverser traverser;
	CASTListOptimizer optimizer;

	traverser.Traverse(root, optimizer);

	CASTDoWhileOptimier doWhileOptimizer;
	traverser.Traverse(root, doWhileOptimizer);
}

