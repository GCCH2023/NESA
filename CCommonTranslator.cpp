#include "stdafx.h"
#include "CCommonTranslator.h"
#include "CGraphTranslator.h"
#include "CDirectTranslator.h"

CCommonTranslator::CCommonTranslator(Allocator& allocator_) :
allocator(allocator_)
{
}

CCommonTranslator::~CCommonTranslator()
{
}

Function* CCommonTranslator::Translate(TACFunction* func)
{
	// 首次尝试控制流图翻译
	auto mark = allocator.Mark();
	try
	{
		CGraphTranslator graphTranslator(allocator);
		return graphTranslator.Translate(func);
	}
	catch (Exception& e)
	{
		Sprintf<> s;
		COUT << s.Format(_T("控制流翻译异常，使用直接翻译: %s\n"), e.Message());
		allocator.Rollback(mark);
		CDirectTranslator translator(allocator);
		return  translator.Translate(func);
	}
}
