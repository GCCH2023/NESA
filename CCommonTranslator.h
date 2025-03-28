#pragma once
class Function;
class TACFunction;

// 通用的翻译方法
class CCommonTranslator
{
public:
	CCommonTranslator(Allocator& allocator);
	~CCommonTranslator();
	Function* Translate(TACFunction* func);
private:
	Allocator& allocator;
};

