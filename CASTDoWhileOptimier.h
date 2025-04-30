#pragma once
#include "CNodeVisitor.h"

// 尝试将 do while 转换为 for
class CASTDoWhileOptimier :
    public CNodeVisitor
{
public:
	CASTDoWhileOptimier();
	void Reset();
protected:
	virtual void OnVisit(Statement* node) override;
	// 获取do while节点的初始化语句，var 是迭代变量
	// 失败返回 nullptr
	Statement* GetInitializeStatement(Statement* node, Expression* var);
	// 查找do while 语句 node 的迭代语句，var 是迭代变量
	// 失败返回 nullptr
	Statement* GetIteratorStatement(Statement* node, Expression* var);
	// 优化DoWhile语句
	void OptimizeDoWhile(Statement* doWhile);
	// 获取父节点
	Statement* GetParent() const { return parents.back(); }
private:
	std::vector<Statement*> parents;
};

