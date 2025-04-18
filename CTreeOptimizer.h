#pragma once
struct CNode;

// 优化抽象语法树的结构
class CTreeOptimizer
{
public:
	CTreeOptimizer();
	~CTreeOptimizer();

	void Optimize(CNode* root);
};

