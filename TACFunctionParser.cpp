#include "stdafx.h"
#include "TACFunctionParser.h"
#include "ReachingDefinition.h"
#include "LiveVariableAnalysis.h"
#include "TACDeadCodeElimination.h"
#include "TACPeephole.h"

TACFunctionParser::TACFunctionParser(NesDataBase& db_):
db(db_)
{
}


TACFunctionParser::~TACFunctionParser()
{
}

void TACFunctionParser::Parse(TACFunction* func)
{
	if (!func)
		return;

	Allocator allocator(4 * 1024 * 1024);
	// 1. 进行到达定值分析，如果AXY能够到达返回基本块，则可能是返回值
	ReachingDefinition rd(db, allocator);
	rd.Analyze(func);

	for (auto block : func->GetBasicBlocks())
	{
		if ((block->flag & BBF_END_MASK) == BBF_END_RETURN)
		{
			auto blockSet = (BasicBlockReachingDefinitionSet*)block->tag;
			if (blockSet->out.set[TAC_REG_A].Any())
				func->flag |= SUBF_RETURN_A;
			if (blockSet->out.set[TAC_REG_X].Any())
				func->flag |= SUBF_RETURN_X;
			if (blockSet->out.set[TAC_REG_Y].Any())
				func->flag |= SUBF_RETURN_Y;
		}
	}

	// 2. 进行优化
	TACPeephole tacPh(db);
	tacPh.Optimize(func);
	//COUT << _T("\n窥孔优化后:\n");
	//func->Dump();

	// 3. 进行死代码消除
	TACDeadCodeElimination tacDce(db, allocator);
	tacDce.Optimize(func);
	//COUT << _T("\n死代码消除后:\n");
	//func->Dump();

	// 4. 如果入口基本块中使用了AXY，则AXY作为参数
	auto entry = *func->GetBasicBlocks().begin();
	auto lives = (BasicBlockLiveVariableSet*)entry->tag;
	func->flag |= (uint32_t)lives->in.ToInteger();

	DumpTACSubroutineAXY(func);
}
