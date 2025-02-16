#pragma once
#include "DataFlowAnalyzer.h"
#include "NodeSet.h"

class TACSubroutine;
class TAC;

struct AXYSet
{
	NodeSet a;
	NodeSet x;
	NodeSet y;

	AXYSet& operator|=(const AXYSet& other);
};

struct BasicBlockReachingDefinitionSet
{
	AXYSet genKill;  // ����ɱ����
	AXYSet in;  // ��ڼ�
	AXYSet out;  // ���ڼ�

	// ������ڼ��������Ƿ����仯
	bool EvalOut();

};

// NES A, X, Y �Ĵ����Ķ�ֵ������
// ������Ƕ�ֵ����ַ�����б��е�����
struct TacAxyDefinition
{
	// AXY �Ķ�ֵ����ַ�������б�
	std::vector<int> adefs;
	std::vector<int> xdefs;
	std::vector<int> ydefs;

	//// ��ȡ �Ĵ��� A ������
	//inline NodeSet GetAMask() const { return (1 << adefs.size()) - 1; }
	//// ��ȡ �Ĵ��� X ������
	//inline NodeSet GetXMask() const { return ((1 << xdefs.size()) - 1) << adefs.size(); }
	//// ��ȡ �Ĵ��� Y ������
	//inline NodeSet GetYMask() const { return ((1 << ydefs.size()) - 1) << (adefs.size() + xdefs.size()); }

	void CheckDefinitionLimit();

	// ��ȡ���ж�ֵ�������ַ��
	void GetDefinitionTACList(std::vector<TAC*>& result, AXYSet& set, TACSubroutine* tacSub);
};


// ���е��ﶨֵ����
class ReachingDefinition : public DataFlowAnalyzer
{
public:
	ReachingDefinition(NesDataBase& db, Allocator& allocator);
	~ReachingDefinition();

	virtual void Initialize() override;
private:
	Allocator& allocator;  // ���ڴ���������
	TacAxyDefinition axyDefs;
protected:
	virtual bool IteraterBasicBlock(TACBasicBlock* block) override;
	void GetAXYDefinitions(TacAxyDefinition& axyDefs, TACSubroutine* tacSub);

	virtual void Uninitialize() override;

};

