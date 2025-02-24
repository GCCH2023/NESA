#pragma once
#include "CTreeVisitor.h"

// 优化抽象语法树的结构
class CTreeOptimizer : public CTreeVisitor
{
public:
	CTreeOptimizer();
	~CTreeOptimizer();

	void Optimize(CNode* root);
protected:
	virtual void OnVisit(CNode* node) override;
	// 尝试合并两条语句，没有合并返回0，合并返回对应的类型
	int TryCombineStatementList(CNode* first, CNode* second);

private:
	std::unordered_set<CNode*> visited;  // 被访问过的节点
};

