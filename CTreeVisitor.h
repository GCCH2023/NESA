#pragma once
struct CNode;

// ���������﷨��
class CTreeVisitor
{
public:
	CTreeVisitor();
	~CTreeVisitor();

	void Visit(CNode* root);

protected:
	// ������һ���ڵ�ʱ
	virtual void OnVisit(CNode* node);
	// ����һ���ڵ���ӽڵ㣬��Ҫ��OnVisit�е���
	void VisitChildren(CNode* node);
};

