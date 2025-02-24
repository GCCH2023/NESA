#pragma once
struct CNode;

// 遍历抽象语法树
class CTreeVisitor
{
public:
	CTreeVisitor();
	~CTreeVisitor();

	void Visit(CNode* root);

protected:
	// 遍历到一个节点时
	virtual void OnVisit(CNode* node);
	// 访问一个节点的子节点，需要在OnVisit中调用
	void VisitChildren(CNode* node);
};

