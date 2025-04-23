#pragma once
#include "CStatementVisitor.h"

// 优化抽象语法树中的列表语句
/*
* + 表示顺序执行，= 表示合并, { 语句 编号 } 表示复合语句，- 表示去除
* “语句 编号” 表示 任意多条顺序执行的语句
* “空语句” 表示什么都不执行的语句
* “{ }” 表示不包含任何语句的复合语句 
（1）{ 语句1, { 语句2 }, 语句3 } = { 语句1, 语句2, 语句3 }
（2）{ 语句1 } = { 语句1 - 空语句 }
（3）{ 语句1 } = 语句1, 语句1只有1条语句
（4）{ } = 空语句
*/
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

