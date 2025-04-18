#include "stdafx.h"
#include "CTreeOptimizer.h"
#include "CASTListOptimizer.h"

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
}

