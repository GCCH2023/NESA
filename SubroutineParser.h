#pragma once
class NesSubroutine;
class NesDataBase;
class NesBasicBlock;
struct Instruction;

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
		// 分析完一个基本块时调用
		// instruction: 基本块的最后一条指令
		virtual void OnEndBasicBlock(NesBasicBlock* block, const Instruction& instruction);
	protected:
		NesDataBase& db;
		NesSubroutine* subroutine;  // 当前分析的子程序
	};
}

