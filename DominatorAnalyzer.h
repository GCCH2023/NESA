#pragma once
#include "TACFunctionAnalyzer.h"

struct Dominator
{
	NodeSet doms;  // 节点的所有支配节点
	int index;  // 节点的索引
};

// 支配节点分析结果
class DominatorResult
{
public:
	DominatorResult(size_t capacity) :
		data(capacity)
	{
	}
	~DominatorResult() = default;
	// 获取指定索引的基本块的支配数据
	inline Dominator& operator[](size_t index) { return data[index]; }
	inline Dominator& Get(size_t index) { return data[index]; }
private:
	std::vector<Dominator> data;
};

// 支配节点分析
// 分析完成后，tag 为 Dominator*
class DominatorAnalyzer:
	public TACFunctionAnalyzer<DominatorResult>
{
public:
	DominatorAnalyzer(NesDataBase& db, Allocator& allocator);
	~DominatorAnalyzer();
protected:
	void Initialize() override;
	bool AnalyzeNode(TACBasicBlock* block) override;
	void Uninitialize() override;

protected:
	NodeSet full;
};

