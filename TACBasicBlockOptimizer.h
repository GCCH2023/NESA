#pragma once
#include "TAC.h"

struct TACNode
{
	TACOperator op;  // 使用 NOP 来表示叶子节点
	union
	{
		struct 
		{
			TACNode* x;
			TACNode* y;
		};
		uint32_t leaf;  // 叶子节点 TACOperand 的内部值
	};
	std::list<TACOperand> attach;  // 附加到这个节点上的变量
	TACOperand result;  // 这个节点最终对应的操作数
};

struct TACNodeHash
{
	std::size_t operator()(const TACNode* node) const;
};

// 自定义比较函数
struct TACNodeEqual
{
	bool operator()(const TACNode* node1, const TACNode* node2) const;
};


// 优化一个基本块中的三地址码
class TACBasicBlockOptimizer
{
public:
	// 用于生成优化结果
	TACBasicBlockOptimizer(Allocator& allocator);
	~TACBasicBlockOptimizer();

	std::vector<TAC*>& Optimize(std::vector<TAC*>& codes);
	
protected:
	// 获取三地址码操作数对应的节点
	TACNode* GetNode(const TACOperand& operand);
	TACNode* GetNode(TACNode* node);
	// 生成节点对应的三地址码，返回保存结果的三地址码操作数
	TACOperand GenerateTAC(std::vector<TAC*>& codes, TACNode* node);
	// 将一个变量附加到指定节点上
	void Attach(TACOperand& var, TACNode* node);
	// 新的临时变量
	TACOperand NewTemp();
protected:
	Allocator& allocator;  // 用于分配优化结果的分配器
	Allocator tempAllocator;

	std::unordered_set<TACNode*, TACNodeHash, TACNodeEqual> tacNodeSet;
	std::unordered_map<uint32_t, TACNode*> varMap;  // 变量最近的赋值节点
	std::vector<TAC*> result;  // 保存结果
	int tempCount;
};

