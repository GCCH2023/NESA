#pragma once
#include "TACOptimizer.h"
#include "TACFunction.h"

class ReachingDefinitionResult;

// 对三地址码进行常量替换，常量折叠，代数优化等操作
class TACPeephole : public TACOptimizer
{
	// 当前基本块，操作数 -> 定值指令 的映射
	using VarDefMap = std::unordered_map<TACOperand, TAC*, TACOperandHash>;

public:
	TACPeephole(NesDataBase& db, std::shared_ptr<ReachingDefinitionResult> rdResult = nullptr);
	~TACPeephole();

	virtual void Optimize(TACFunction* subroutine) override;
	// 重置算法用到的数据
	virtual void Reset() override;
protected:
	// 设置操作数的定值指令
	void SetOperandDefinition(TAC* tac);
	// 获取操作数的定值指令
	// 要求操作数是寄存器或临时变量
	// 如果操作数在其他基本块定值，则返回nullptr
	virtual TAC* GetOperandDefinition(TACOperand& operand);
	// 判断操作数的值是否发生改变
	virtual bool IsOperandChanged(TACOperand& operand, TAC* current);
	// 进行代数优化
	void OptimizeExpression(TACOperand& operand, TACOperand& other, TAC* current, TAC* tac);
	void TryReplaceOperand(TACOperand& operand);
	void TryReplaceOperand(TACOperand& operand, TACOperand& other, TAC* current);
	inline TACBasicBlock* GetCurrentBasicBlock() { return currentBlock; }
private:
	VarDefMap varDefMap;
	TACBasicBlock* currentBlock;  // 当前处理的基本块
	std::shared_ptr<ReachingDefinitionResult> reachDefResult;
};

