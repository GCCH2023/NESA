#pragma once
#include "TACOptimizer.h"

enum class TACValueKind
{
	Undefined,  // 未定义
	Constant,  // 常量
	Boolean,  // 布尔表达式
	Unknown,  // 不是常量
};

struct TACValue
{
	TACValueKind kind;
	TAC* tac;  // 定值指令
};


// 对三地址进行窥孔优化
class TACPeephole : public TACOptimizer
{
public:
	TACPeephole(NesDataBase& db);
	~TACPeephole();

	virtual void Optimize(TACFunction* subroutine) override;
	// 重置算法用到的数据
	virtual void Reset() override;
};

