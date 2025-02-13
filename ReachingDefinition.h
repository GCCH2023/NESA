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
	AXYSet genKill;  // 生成杀死集
	AXYSet in;  // 入口集
	AXYSet out;  // 出口集

	// 计算出口集，返回是否发生变化
	bool EvalOut();

};

// NES A, X, Y 寄存器的定值点索引
// 保存的是定值三地址码在列表中的索引
struct TacAxyDefinition
{
	// AXY 的定值三地址码索引列表
	std::vector<int> adefs;
	std::vector<int> xdefs;
	std::vector<int> ydefs;

	//// 获取 寄存器 A 的掩码
	//inline BitSet32 GetAMask() const { return (1 << adefs.size()) - 1; }
	//// 获取 寄存器 X 的掩码
	//inline BitSet32 GetXMask() const { return ((1 << xdefs.size()) - 1) << adefs.size(); }
	//// 获取 寄存器 Y 的掩码
	//inline BitSet32 GetYMask() const { return ((1 << ydefs.size()) - 1) << (adefs.size() + xdefs.size()); }

	void CheckDefinitionLimit();

	// 获取所有定值点的三地址码
	void GetDefinitionTACList(std::vector<TAC*>& result, AXYSet& set, TACSubroutine* tacSub);
};


// 进行到达定值分析
class ReachingDefinition : public DataFlowAnalyzer
{
public:
	ReachingDefinition(NesDataBase& db, Allocator& allocator);
	~ReachingDefinition();

	virtual void Initialize() override;
private:
	Allocator& allocator;  // 用于创建输出结果
	TacAxyDefinition axyDefs;
protected:
	virtual bool IteraterBasicBlock(TACBasicBlock* block) override;
	void GetAXYDefinitions(TacAxyDefinition& axyDefs, TACSubroutine* tacSub);

	virtual void Uninitialize() override;

};

