#pragma once
#include "DataFlowAnalyzer.h"
#include "TACFunction.h"
#include "NesDataBase.h"
#include "NodeSet.h"

class Allocator;

// 三地址码函数数据流分析
class TACFunctionAnalyzer:
	public DataFlowAnalyzer<TACFunction, TACBasicBlock>
{
public:
	// allocator : 用于分配创建输出结果
	TACFunctionAnalyzer(NesDataBase& db, Allocator& allocator);
	~TACFunctionAnalyzer();

protected:
	// 获取三地址码函数
	inline TACFunction* GetFunction() { return graph; }
	virtual const std::vector<TACBasicBlock *>& GetAllNodes() override;
protected:
	NesDataBase& db;
	Allocator& allocator;
};

