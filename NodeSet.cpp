#include "stdafx.h"
#include "NodeSet.h"
using namespace std;


void DumpNodeSet(NodeSet& bs)
{
	auto vec = bs.ToVector();
	if (vec.empty())
	{
		COUT << _T("¿Õ");
		return;
	}

	for (auto i : vec)
	{
		COUT << i << _T(", ");
	}
}

void BasicBlock::Dump()
{
	COUT << _T("block ") << index << _T(" , Ç°Çý : ");
	DumpNodeSet(pred);
	COUT << _T(" ºó¼Ì : ");
	DumpNodeSet(succ);
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