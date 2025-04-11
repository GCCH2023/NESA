#pragma once
class NesSubroutine;
class NesDataBase;
class NesBasicBlock;
struct Instruction;

namespace NesDB
{
	// 从指定地址开始解析子程序
	// 将跳转到的所有基本块纳入子程序范围
	// 不会添加基本块到数据库中，只添加子程序到数据库中
	class SubroutineParser
	{
	public:
		SubroutineParser(NesDataBase& db);
		NesSubroutine* Parse(Nes::Address start);
		void Reset();
	protected:
		// 根据地址获取获取包含这个地址的基本块或者这个地址后面的第一个基本块
		NesBasicBlock* GetBasicBlockOrNext(Nes::Address address);
		// 添加新的基本块
		void AddBasicBlock(NesBasicBlock* block);
		// 给定地址和已分析过的基本块的特殊处理
		// 返回 0 表示不需要继续分析给定地址
		// 返回非0 表示给定地址的最大结束地址
		Nes::Address ParseExist(Nes::Address address);
	protected:
		NesDataBase& db;
		NesSubroutine* subroutine;  // 当前分析的子程序
		std::vector<Nes::Address> queue;  // 基本块开始地址队列
		std::map<Nes::Address, NesBasicBlock*> blocks;  // 子程序包含的所有基本块
	};
}

