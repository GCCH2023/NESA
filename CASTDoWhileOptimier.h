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
};

