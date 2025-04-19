#pragma once
#include "CASTContextTraverser.h"

// 尝试将 do while 转换为 for
class CASTDoWhileOptimier :
    public CASTContextVisitor
{
public:
	using CASTContextVisitor::CASTContextVisitor;
	void Reset();
protected:
	virtual void PreVisit(CNode* node, int depth) override;
	// 检查条件表达式是否符合转换条件
	bool CheckCondition(const CNode* node);
	// 获取do while节点的初始化语句，var 是迭代变量
	// 失败返回 nullptr
	CNode* GetInitializeStatement(CNode* node, CNode* var);
	// 查找do while 语句 node 的迭代语句，var 是迭代变量
	// 失败返回 nullptr
	CNode* GetIteratorStatement(CNode* node, CNode* var);

};

