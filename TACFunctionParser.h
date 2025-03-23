#pragma once
class NesDataBase;
class TACFunction;

// 对 TAC 函数进行参数和返回值的分析，
// 优化 其中的代码
class TACFunctionParser
{
public:
	TACFunctionParser(NesDataBase& db);
	~TACFunctionParser();
	void Parse(TACFunction* func);
protected:
	NesDataBase& db;
};

