#pragma once
#include "CStatementVisitor.h"

// 优化抽象语法树中的列表语句
class CASTListOptimizer : public CStatementVisitor
{
public:
	CASTListOptimizer() = default;
	void Reset();
protected:
	virtual void OnVisit(CNode* node) override;
	// 尝试合并两条语句，没有合并返回0，合并返回对应的类型
	int TryCombineStatementList(CNode* first, CNode* second);
	// 尝试优化语句列表节点
	void TryOptimizeStatementList(CNode* node);
private:
	std::unordered_set<CNode*> visited;  // 被访问过的节点
};

