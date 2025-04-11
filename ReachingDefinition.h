#pragma once
#include "TACFunctionAnalyzer.h"

// AXYNVZC的定值点依次排列
// 如果A有n个定值点，则Set前n个元素对应A的定值点
using Set = boost::dynamic_bitset<>;

struct BasicBlockReachingDefinitionSet
{
	Set gen;  // 生成的定值点集合
	Set kill;  // 杀死的定值点集合
	Set in;  // 入口集
	Set out;  // 出口集

	// 计算出口集，返回是否发生变化
	bool EvalOut();

};

// NES A, X, Y 寄存器的定值点索引
// 保存的是定值三地址码在列表中的索引
struct TacAxyDefinition
{
	// AXY NVZC 的定值三地址码索引列表
	std::vector<int> defs[TAC_ANALIZE_REG_COUNT];

	void CheckDefinitionLimit(TACFunction* tacSub);

	// 获取所有定值点的三地址码
	void GetDefinitionTACList(std::vector<TAC*>& result, Set& set, TACFunction* tacSub);
};


// 进行到达定值分析
// 基本块的 tag 设置为 BasicBlockReachingDefinitionSet 指针
class ReachingDefinition:
	public TACFunctionAnalyzer
{
public:
	ReachingDefinition(NesDataBase& db, Allocator& allocator);
	~ReachingDefinition();
	// 对于基本块的入口集或出口集，判断指定AXYNVZC变量是否可以到达
	bool CanReach(const Set& set, int var);
private:
	TacAxyDefinition axyDefs;
	Set masks[TAC_ANALIZE_REG_COUNT];  // AXYNVZC 对应位区间的掩码
	size_t count;  // 变量的定值点总数
protected:
	virtual void Initialize() override;
	virtual bool AnalyzeNode(TACBasicBlock* block) override;
	virtual void Uninitialize() override;

	void GetDefinitions(TacAxyDefinition& axyDefs);
	void GenerateMask();  // 计算掩码
	void GetBasickBlockGenKillMap();
};

