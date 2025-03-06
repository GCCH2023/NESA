#pragma once
class TACFunction;
class NesDataBase;
class TACBasicBlock;

// 数据流分析的基类
// 传递指定类型的参数来进行分析
// 在分析函数调用指令时，需要知道调用的函数是否引用了寄存器
// template<typename TParam>
class DataFlowAnalyzer
{
public:
	DataFlowAnalyzer(NesDataBase& db);
	~DataFlowAnalyzer();
	// 进行分析
	virtual void Analyze(TACFunction* subroutine);
protected:
	// 初始化
	virtual void Initialize() = 0;
	// 迭代基本块，返回是否结束迭代，所有基本块结束才结束
	virtual bool IteraterBasicBlock(TACBasicBlock* block) = 0;
	// 反初始化
	virtual void Uninitialize();

	NesDataBase& db;
	TACFunction* subroutine;
};

