#pragma once

class CNode;
class Statement;
class Expression;

// 遍历抽象语法树中的语句和表达式
// 需要调用 VisitChildren 来访问子节点
// 默认实现是访问子节点
class CNodeVisitor
{
public:
	CNodeVisitor() = default;
	virtual ~CNodeVisitor() = default;

	void Visit(CNode* root);
protected:
	// 访问指定节点的所有子语句节点
	void VisitChildren(CNode* node);
	// 开始遍历前调用，返回是否继续遍历
	virtual bool BeforeVisit(CNode* root);
	// 遍历完成后调用
	virtual void AfterVisit(CNode* root);
	// 访问语句节点
	virtual void OnVisit(Statement* node);
	// 访问表达式节点
	virtual void OnVisit(Expression* node);
};

