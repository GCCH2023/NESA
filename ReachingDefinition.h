#pragma once
#include "TACFunctionAnalyzer.h"

// NES A, X, Y 寄存器的定值点索引
// 保存的是定值三地址码在列表中的索引
class ReachingDefinitionResult
{
private:
	friend class ReachingDefinition;

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

public:
	ReachingDefinitionResult(TACFunction* func);
	// 输出基本块的定值点
	void DumpBasicBlockDefinitions(size_t blockIndex) const;
	// 输出所有基本块的定值点
	void DumpAllBasicBlockDefinitions() const;
	// 输出寄存器的所有定值点
	void DumpRegisterDefinitions() const;
	// 对于基本块的入口集或出口集，判断指定AXYNVZC变量是否可以到达
	bool CanReach(size_t blockIndex, bool isIn, int var);
	// 获取所有定值点的三地址码
	void GetDefinitionTACList(std::vector<TAC*>& result, const Set& set) const;
	// 获取可以到达指定基本块入口的所有定值三地址码
	void GetBasicBlockDefinitionsIn(std::vector<TAC*>& result, size_t blockIndex) const;
	// 获取可以到达指定基本块入口的指定变量的定值三地址码
	void GetBasicBlockDefinitionsIn(std::vector<TAC*>& result, size_t blockIndex, const TACOperand& var) const;
protected:
	void GenerateMask();  // 计算掩码
private:
	TACFunction* function;
	// AXY NVZC 的定值三地址码索引列表
	std::vector<int> defs[TAC_ANALIZE_REG_COUNT];
	// 每个基本块的分析数据
	std::vector<BasicBlockReachingDefinitionSet> data;
	Set masks[TAC_ANALIZE_REG_COUNT];  // AXYNVZC 对应位区间的掩码
	size_t count;  // 变量的定值点总数
};

// 进行到达定值分析
class ReachingDefinition:
	public TACFunctionAnalyzer<ReachingDefinitionResult>
{
public:
	ReachingDefinition(NesDataBase& db, Allocator& allocator);
	~ReachingDefinition();
protected:
	virtual void Initialize() override;
	virtual bool AnalyzeNode(TACBasicBlock* block) override;
	virtual void Uninitialize() override;

	void GetDefinitions();
	void GetBasickBlockGenKillMap();
};

