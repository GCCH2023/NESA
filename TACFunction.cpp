#include "stdafx.h"
#include "TACFunction.h"


TACBasicBlock::TACBasicBlock()
{

}

TACBasicBlock::TACBasicBlock(Nes::Address startAddress, Nes::Address endAddress) :
NesRegion(startAddress, endAddress)
{

}


TACFunction::TACFunction()
{

}

TACFunction::TACFunction(Nes::Address startAddress, Nes::Address endAddress) :
NesRegion(startAddress, endAddress)
{

}

void TACFunction::Dump()
{
	for (auto tac : GetCodes())
	{
		DumpAddressTAC(COUT, tac) << std::endl;
	}
}

TACList TACFunction::GetCodes()
{
	TACList tacs;
	for (auto block : GetBasicBlocks())
	{
		auto& codes = block->GetCodes();
		tacs.insert(tacs.end(), codes.begin(), codes.end());
	}
	return tacs;
}

int TACFunction::NewTemp(Type* type)
{
	assert(type);
	int no = (int)tempTypes.size();
	tempTypes.push_back(type);
	return no;
}

void DumpTACSubroutineAXY(TACFunction* sub)
{
	static const TCHAR* axyStr[] =
	{
		_T("无"), _T("A"), _T("X"), _T("AX"), _T("Y"), _T("AY"), _T("XY"), _T("AXY")
	};
	static int count = 0;

	int params = sub->flag & 7;
	int returns = (sub->flag >> 3) & 7;
	TCHAR buffer[128];

	_stprintf_s(buffer, _T("%d 函数 %04X 的分析结果，参数: %s，返回值: %s\n"), count++, sub->GetStartAddress(),
		axyStr[params], axyStr[returns]);
	COUT << buffer;
}