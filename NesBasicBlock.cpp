#include "stdafx.h"
#include "NesBasicBlock.h"


NesBasicBlock::NesBasicBlock()
{

}


NesBasicBlock::NesBasicBlock(Nes::Address startAddr, Nes::Address endAddr):
NesRegion(startAddr, endAddr)
{

}

NesBasicBlock::NesBasicBlock(const NesBasicBlock& other):
NesRegion(other.GetStartAddress(), other.GetEndAddress()),
flag(other.flag),
prevs(other.prevs)
{
	nexts[0] = other.nexts[0];
	nexts[1] = other.nexts[1];
}

void NesBasicBlock::AddPrev(NesBasicBlock* block)
{
	this->prevs.push_back(block);
}

size_t NesBasicBlock::GetNextCount() const
{
	if (nexts[1])
		return 2;
	return nexts[0] ? 1 : 0;
}

void NesBasicBlock::Dump()
{
	Sprintf<> s;
	s.Format(_T("\n==== 基本块 %04X:"), this->GetStartAddress());
	s.Append(_T("前驱 = "));
	if (this->prevs.empty())
		s.Append(_T("null"));
	else
	{
		for (auto prev : this->prevs)
		{
			s.Append(_T("%04X, "), prev->GetStartAddress());
		}
	}
	s.Append(_T("  后继 = "));
	if (this->nexts[0])
	{
		s.Append(_T("%04X"), this->nexts[0]->GetStartAddress());
		if (this->nexts[1])
			s.Append(_T(", %04X"), this->nexts[1]->GetStartAddress());
	}
	else
	{
		s.Append(_T("null"));
	}
	s.Append(_T("\n"));
	COUT << s.ToString();
}
