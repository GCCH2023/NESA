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
	CASTListOptimizer optimizer;
	optimizer.Visit(root);

	CASTDoWhileOptimier doWhileOptimizer;
	doWhileOptimizer.Visit(root);
}

