#pragma once
#include "TACFunctionAnalyzer.h"

struct AXYSet
{
	NodeSet set[TAC_ANALIZE_REG_COUNT];  // AXY NVZC 7个的集合

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
	// AXY NVZC 的定值三地址码索引列表
	std::vector<int> defs[TAC_ANALIZE_REG_COUNT];

	//// 获取 寄存器 A 的掩码
	//inline NodeSet GetAMask() const { return (1 << adefs.size()) - 1; }
	//// 获取 寄存器 X 的掩码
	//inline NodeSet GetXMask() const { return ((1 << xdefs.size()) - 1) << adefs.size(); }
	//// 获取 寄存器 Y 的掩码
	//inline NodeSet GetYMask() const { return ((1 << ydefs.size()) - 1) << (adefs.size() + xdefs.size()); }

	void CheckDefinitionLimit(TACFunction* tacSub);

	// 获取所有定值点的三地址码
	void GetDefinitionTACList(std::vector<TAC*>& result, AXYSet& set, TACFunction* tacSub);
};


// 进行到达定值分析
// 基本块的 tag 设置为 BasicBlockReachingDefinitionSet 指针
class ReachingDefinition:
	public TACFunctionAnalyzer
{
public:
	ReachingDefinition(NesDataBase& db, Allocator& allocator);
	~ReachingDefinition();

private:
	TacAxyDefinition axyDefs;
protected:
	virtual void Initialize() override;
	virtual bool AnalyzeNode(TACBasicBlock* block) override;
	virtual void Uninitialize() override;

	void GetAXYDefinitions(TacAxyDefinition& axyDefs, TACFunction* tacSub);
};

