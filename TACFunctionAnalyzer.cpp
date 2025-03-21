#include "stdafx.h"
#include "TACFunctionAnalyzer.h"


TACFunctionAnalyzer::TACFunctionAnalyzer(NesDataBase& db_, Allocator& allocator_):
db(db_),
allocator(allocator_)
{
}

TACFunctionAnalyzer::~TACFunctionAnalyzer()
{
}


const std::vector<TACBasicBlock *>& TACFunctionAnalyzer::GetAllNodes()
{
	return GetFunction()->GetBasicBlocks();
}
