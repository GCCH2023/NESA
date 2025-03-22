#include "stdafx.h"
#include "NodeSet.h"
using namespace std;


void DumpNodeSet(NodeSet& bs)
{
	auto vec = bs.ToVector();
	if (vec.empty())
	{
		COUT << _T("Пе");
		return;
	}

	for (auto i : vec)
	{
		COUT << i << _T(", ");
	}
}

const TCHAR* ToString(CtrlTreeNodeType region)
{
	static const TCHAR* names[] =
	{
		_T("LEAF"), _T("LIST"), _T("SELF-LOOP"), _T("IF"), _T("IF-ELSE"), _T("IFOR"), _T("POINT2-LOOP")
	};
	return names[region];
}