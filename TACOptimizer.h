#pragma once
#include "TACFunction.h"

class NesDataBase;

// 优化三地址码
class TACOptimizer
{
public:
	TACOptimizer(NesDataBase& db);
	~TACOptimizer();
	// 执行优化
	virtual void Optimize(TACFunction* subroutine);
	// 重置优化器
	virtual void Reset();
protected:
	NesDataBase& db;
};

// 判断操作数是不是AXYNVZC
inline bool IsAxyNvzc(TACOperand& operand)
{
	return operand.IsRegister() && operand.GetValue() <= TAC_REG_C;
}

// 判断操作数是不是AXYNVZC 或临时变量
inline bool IsAxyNvzcTemp(TACOperand& operand)
{
	return operand.IsRegister() && operand.GetValue() <= TAC_REG_C || operand.IsTemp();
}

// 获取AXYNVZC 或临时变量的索引
// 寄存器的索引从 0 开始，之后才是临时变量索引
inline int GetAxyNvzcTempIndex(TACOperand& operand)
{
	return operand.IsTemp() ? operand.GetValue() + TAC_ANALIZE_REG_COUNT : operand.GetValue();
}
