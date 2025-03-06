#include "stdafx.h"
#include "DataFlowAnalyzer.h"
#include "TACFunction.h"
#include "NesBasicBlock.h"

DataFlowAnalyzer::DataFlowAnalyzer(NesDataBase& db_):
db(db_)
{

}

DataFlowAnalyzer::~DataFlowAnalyzer()
{
}

void DataFlowAnalyzer::Analyze(TACFunction* subroutine)
{
	if (!subroutine)
		return;
	this->subroutine = subroutine;

	Initialize();

	bool isEnd = false;
	// 迭代
	int iter = 0;
	while (!isEnd)
	{
		isEnd = true;
		bool isEntry = true;
		// 遍历基本块
		for (auto block : this->subroutine->GetBasicBlocks())
		{
			if (!IteraterBasicBlock(block))
				isEnd = false;
		}
	}

	Uninitialize();
}

void DataFlowAnalyzer::Uninitialize()
{

}
