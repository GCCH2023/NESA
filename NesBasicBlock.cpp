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
preds(other.preds),
succs(other.succs)
{
}

void NesBasicBlock::Split(Nes::Address addr, NesBasicBlock* succ)
{
	if (addr <= GetStartAddress() || addr >= GetEndAddress())
		return;
	succ->SetStartAddress(addr);
	succ->SetEndAddress(GetEndAddress());
	succ->succs = succs;
	succ->preds.clear();
	succ->preds.push_back(GetStartAddress());

	SetEndAddress(addr);
	succs.clear();
	succs.push_back(succ->GetStartAddress());
}

bool NesBasicBlock::IsPred(Nes::Address addr) const
{
	auto it = std::find(preds.begin(), preds.end(), addr);
	return it != preds.end();
}

bool NesBasicBlock::IsSucc(Nes::Address addr) const
{
	auto it = std::find(succs.begin(), succs.end(), addr);
	return it != succs.end();
}

void NesBasicBlock::Dump()
{
	Sprintf<> s;
	s.Format(_T("==== 基本块 %04X:"), this->GetStartAddress());
	s.Append(_T("前驱 = "));
	if (this->preds.empty())
		s.Append(_T("null"));
	else
	{
		for (auto prev : this->preds)
		{
			s.Append(_T("%04X, "), prev);
		}
	}
	s.Append(_T("  后继 = "));
	if (this->succs.empty())
		s.Append(_T("null"));
	else
	{
		for (auto succ : this->succs)
		{
			s.Append(_T("%04X, "), succ);
		}
	}
	s.Append(_T("\n"));
	COUT << s.ToString();
}
