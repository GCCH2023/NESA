#pragma once

struct CNode;

// 访问 抽象语法树中的所有语句
class CStatementVisitor
{
public:
	CStatementVisitor() = default;
	virtual ~CStatementVisitor() {}

	void Visit(CNode* root);
protected:
	// 访问指定节点的所有子语句节点
	void VisitChildren(CNode* node);
	// 访问指定节点，调用VisitChildren以访问它的子节点
	virtual void OnVisit(CNode* node);
};

