#include "stdafx.h"
#include "NodeSet.h"
using namespace std;

BitSet32::BitSet32():
data(0)
{

}

BitSet32::BitSet32(uint32_t value):
data(value)
{

}

BitSet32::BitSet32(std::initializer_list<int> values):
data(0)
{
	for (auto v : values)
	{
		*this += v;
	}
}

std::vector<int> BitSet32::ToVector() const
{
	uint32_t s = data;
	std::vector<int> vec(16);
	vec.clear();
	for (int i = 0; s; ++i)
	{
		if (s & 1)
		{
			vec.push_back(i);
		}
		s >>= 1;
	}
	return vec;
}

int BitSet32::GetSize() const
{
	uint32_t s = data;
	int n = 0;
	while (s)
	{
		if (s & 1)
			++n;
		s >>= 1;
	}
	return n;
}

void BitSet32::Dump() const
{
	uint32_t s = data;
	if (!s)
	{
		COUT << _T("¿Õ");
		return;
	}

	int i = 0;
	while (s)
	{
		if (s & 1)
		{
			COUT << i << _T(", ");
		}
		s >>= 1;
		++i;
	}
}

void BasicBlock::Dump()
{
	COUT << _T("block ") << index << _T(" , Ç°Çý : ");
	pred.Dump();
	COUT << _T(" ºó¼Ì : ");
	succ.Dump();
	COUT << endl;
}

const TCHAR* ToString(CtrlTreeNodeType region)
{
	static const TCHAR* names[] =
	{
		_T("LEAF"), _T("LIST"), _T("SELF-LOOP"), _T("IF"), _T("IF-ELSE"), _T("IFOR"), _T("POINT2-LOOP")
	};
	return names[region];
}