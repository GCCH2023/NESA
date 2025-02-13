#pragma once
#include "DataFlowAnalyzer.h"
#include "NodeSet.h"

class TACSubroutine;
class TAC;

struct AXYSet
{
	BitSet32 a;
	BitSet32 x;
	BitSet32 y;

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
	//inline BitSet32 GetAMask() const { return (1 << adefs.size()) - 1; }
	//// ��ȡ �Ĵ��� X ������
	//inline BitSet32 GetXMask() const { return ((1 << xdefs.size()) - 1) << adefs.size(); }
	//// ��ȡ �Ĵ��� Y ������
	//inline BitSet32 GetYMask() const { return ((1 << ydefs.size()) - 1) << (adefs.size() + xdefs.size()); }

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

