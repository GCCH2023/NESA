#pragma once
class NesDataBase;
struct Instruction;
class NesSubroutine;
class NesBasicBlock;
struct CallRelation;

// 解析 NES 子程序
class NesSubroutineParser
{
public:
	NesSubroutineParser(NesDataBase& db);
	~NesSubroutineParser();

	// 将指定地址解析为子程序
	virtual NesSubroutine* Parse(Nes::Address address);
	// 重置解析器，这样就可以用一个解析器对象来多次解析子程序了
	void Reset();
	// 输出子程序的信息
	void Dump();
protected:
	// 当需要分析一条指令的时候调用，返回值表示是否继续分析下一条指令
	virtual bool ParseInstruction(const Instruction& instruction);
	// 添加指定地址作为基本块开始地址 （从小到大排列并且去重）
	void AddBasicBlockStartAddress(Nes::Address address);
	// 判断地址是否是基本块开始地址
	bool IsBlockStartAddress(Nes::Address address);
	// 验证基本块地址是否在函数内部
	bool CheckBlocksAddress(Nes::Address start, Nes::Address end);
	// 解析基本块之间的连接关系
	void ParseBasicBlocks();
	// 解析基本块的所有指令
	void ParseBasicBlockInstructions(NesBasicBlock* block);
	// 解析基本块的单条指令
	// nextAddr 是后面一条指令的地址
	void ParseBasicBlockInstruction(NesBasicBlock* block, const Instruction& instruction, Nes::Address nextAddr);
	// 设置两个基本块的前后关系
	void BindBlock(NesBasicBlock* prev, Nes::Address nextAddr);
	// 设置基本块的跳转相关标志
	void SetBasickBlockJumpFlag(NesBasicBlock* block, bool isCond, Nes::Address jumpAddr);
	// 保存被调用子程序的地址
	inline void AddCalledAddress(Nes::Address calledAddr) { calls.push_back(calledAddr); }
protected:
	NesDataBase& db;
	std::vector<Nes::Address> blockStartAddrs;  // 基本块开始地址列表
	NesSubroutine* subroutine;
	std::vector<Nes::Address> calls;
};

