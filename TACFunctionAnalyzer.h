#pragma once
#include "DataFlowAnalyzer.h"
#include "TACFunction.h"
#include "NesDataBase.h"
#include "NodeSet.h"
#include <boost/dynamic_bitset/dynamic_bitset.hpp>

class Allocator;

// 三地址码函数数据流分析
template<typename Result>
class TACFunctionAnalyzer:
	public DataFlowAnalyzer<TACFunction, TACBasicBlock, Result>
{
public:
	// allocator : 用于分配创建输出结果
	TACFunctionAnalyzer(NesDataBase& db_, Allocator& allocator_):
		db(db_),
		allocator(allocator_)
	{
	}
	~TACFunctionAnalyzer() = default;
protected:
	// 获取三地址码函数
	inline TACFunction* GetFunction() { return DataFlowAnalyzer<TACFunction, TACBasicBlock, Result>::graph; }
	virtual const std::vector<TACBasicBlock *>& GetAllNodes() override
	{
		return GetFunction()->GetBasicBlocks();
	}
protected:
	NesDataBase& db;
	Allocator& allocator;
};

