#pragma once
class NesSubroutine;
class NesDataBase;

namespace NesDB
{
	// 从指定地址开始解析子程序
	// 将跳转到的所有基本块纳入子程序范围
	class SubroutineParser
	{
	public:
		SubroutineParser(NesDataBase& db);
		NesSubroutine* Parse(Nes::Address start);
		void Reset();

	protected:
		NesDataBase& db;
	};
}

