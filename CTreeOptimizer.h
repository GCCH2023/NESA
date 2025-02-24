#pragma once
#include "CTreeVisitor.h"

// �Ż������﷨���Ľṹ
class CTreeOptimizer : public CTreeVisitor
{
public:
	CTreeOptimizer();
	~CTreeOptimizer();

	void Optimize(CNode* root);
protected:
	virtual void OnVisit(CNode* node) override;
	// ���Ժϲ�������䣬û�кϲ�����0���ϲ����ض�Ӧ������
	int TryCombineStatementList(CNode* first, CNode* second);

private:
	std::unordered_set<CNode*> visited;  // �����ʹ��Ľڵ�
};

