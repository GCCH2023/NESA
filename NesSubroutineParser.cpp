#include "stdafx.h"
#include "NesSubroutineParser.h"
#include "Instruction.h"
#include "NesDataBase.h"
using namespace Nes;

// 用于进行基本块划分的指令分类
enum class BlockInstructionKind
{
	Normal = 0,  // 平常指令
	End,  // 结束基本块的指令
	UnconditionalJump,  // 无条件跳转指令
	ConditionalJump,  // 条件跳转指令
	Call, // 子程序调用指令
};

bool isInitedBlockInstructionMap = false;
// 指令的第一个字节索引的指令分类表
BlockInstructionKind blockInstructionMap[256] = { BlockInstructionKind::Normal };

NesSubroutineParser::NesSubroutineParser(NesDataBase& db_) :
db(db_),
blockStartAddrs(32)
{
	if (!isInitedBlockInstructionMap)
	{
		for (int i = 0; i < 256; ++i)
		{
			const OpcodeEntry& entry = GetOpcodeEntry(i);
			if ((entry.kind & OpKind::Return) != 0 || entry.opcode == Opcode::None)
				blockInstructionMap[i] = BlockInstructionKind::End;
			else if ((entry.kind & OpKind::UnconditionalJump) != 0)
				blockInstructionMap[i] = BlockInstructionKind::UnconditionalJump;
			else if ((entry.kind & OpKind::ConditionalJump) != 0)
				blockInstructionMap[i] = BlockInstructionKind::ConditionalJump;
			else if ((entry.kind & OpKind::Call) != 0)
				blockInstructionMap[i] = BlockInstructionKind::Call;
		}

		isInitedBlockInstructionMap = true;
	}
}


NesSubroutineParser::~NesSubroutineParser()
{

}

// 假设子程序的指令都是连续存放的
// 第一步：从子程序开头开始，一直取指令直到遇到结束指令
// 在取指令的过程中，将跳转目标地址当作基本块开始地址加入到列表中
// 在遇到结束指令时，判断下一条指令是否是基本块开始指令，如果是
// 就继续分析，不是则结束子程序的分析
// 第二步：构建基本块网络
NesSubroutine* NesSubroutineParser::Parse(Nes::Address address)
{
	blockStartAddrs.clear();
	blockStartAddrs.push_back(address);  // 最开始的时候只有函数开始地址

	Instruction instruction;
	// 从卡带中获取函数开头指针
	const uint8_t* p = db.GetCartridge().GetData(address);
	Nes::Address current = address;  // 当前分析的指令地址
	int bytes;
	// 首先划分基本块
	for (;; p += bytes)
	{
		instruction.Set(current, p);  // 构造指令对象
		bytes = instruction.GetLength();
		current += bytes;  // 计算下一条指令的地址
		if (!ParseInstruction(instruction))  // 分析指令
		{
			// 遇到结束指令的时候，如果下一条指令是基本块开始的话，仍然继续分析
			if (!IsBlockStartAddress(current))
				break;
		}
	}

	this->subroutine = db.allocator.New<NesSubroutine>(address, current);

	if (!CheckBlocksAddress(address, current))
	{
		TCHAR buffer[64];
		_stprintf_s(buffer, _T("子程序 %04X, 基本块超出函数范围"), address);
		throw Exception(buffer);
	}

	// 连接基本块，构成控制流图
	ParseBasicBlocks();

	db.AddSubroutine(this->subroutine);
	for (auto called : calls)
	{
		auto callRelation = db.allocator.New<CallRelation>(this->subroutine->GetStartAddress(), called);
		db.AddCallRelation(callRelation);
	}
	return this->subroutine;
}

void NesSubroutineParser::Reset()
{
	blockStartAddrs.clear();
	this->subroutine = nullptr;
	this->calls.clear();
}

void NesSubroutineParser::Dump()
{
	Sprintf<> s;
	COUT << s.Format(_T("函数 %04X 的结束地址为: %04X\n"), subroutine->GetStartAddress(), subroutine->GetEndAddress());
	s.Clear();
	Instruction instruction;
	for (auto block : subroutine->GetBasicBlocks())
	{
		block->Dump();
		for (auto addr = block->GetStartAddress(); addr < block->GetEndAddress();)
		{
			auto p = db.GetCartridge().GetData(addr);
			instruction.Set(addr, p);


			COUT << s.Format(_T("%04X    %s\n"), instruction.GetAddress(), FormatInstruction(instruction));
			s.Clear();
			int bytes = instruction.GetLength();
			p += bytes;
			addr += bytes;
		}
	}
}

bool NesSubroutineParser::ParseInstruction(const Instruction& instruction)
{
	BlockInstructionKind kind = blockInstructionMap[instruction.GetOperatorByte()];
	switch (kind)
	{
	case BlockInstructionKind::Normal:
	case BlockInstructionKind::Call:
		return true;
	case BlockInstructionKind::End:
		return false;  // 子程序结束指令
	case BlockInstructionKind::ConditionalJump:
	{
													// 处理条件为假的情况, 条件为假则下一条指令是新的基本块
													int length = instruction.GetLength();
													Nes::Address address = instruction.address + length;
													AddBasicBlockStartAddress(address);
													// 处理条件为真的情况
													address += (char)instruction.GetByte();
													AddBasicBlockStartAddress(address);
													return true;
	}
	case BlockInstructionKind::UnconditionalJump:
	{
												  auto& entry = instruction.GetEntry();
												  Nes::Address jumpAddr;
												  if (entry.addrMode == AddrMode::Absolute)
													  jumpAddr = instruction.GetOperandAddress();
												  else
												  {
													  TCHAR buffer[128];
													  _stprintf_s(buffer, 128, _T("地址为 %04X 的无条件跳转指令的非绝对寻址模式未实现"), instruction.address);
													  throw Exception(buffer);
												  }

												  AddBasicBlockStartAddress(jumpAddr);
												  return false;  // 无条件跳转后面的指令不会被执行到了，所以不必继续分析
	}
	}
	return true;
}

void NesSubroutineParser::AddBasicBlockStartAddress(Nes::Address address)
{
	// 使用 std::lower_bound 查找插入位置
	auto it = std::lower_bound(blockStartAddrs.begin(), blockStartAddrs.end(), address);

	// 检查该位置是否已经存在相同的值
	if (it == blockStartAddrs.end() || *it != address)
		// 如果不存在相同的值，则插入新值
		blockStartAddrs.insert(it, address);
}

bool NesSubroutineParser::IsBlockStartAddress(Nes::Address address)
{
	// 使用 std::lower_bound 查找插入位置
	auto it = std::lower_bound(blockStartAddrs.begin(), blockStartAddrs.end(), address);
	return it != blockStartAddrs.end() && *it == address;
}

// 检测所有基本块是否位于子程序地址范围内
// !! 由于有可能子程序以JMP结尾，跳到一个较远的地址，也就是尾调用
// 所以，不再做这个检测
bool NesSubroutineParser::CheckBlocksAddress(Nes::Address start, Nes::Address end)
{
	/*if (blockStartAddrs.empty())
		return true;
		Nes::Address firstAddr = *blockStartAddrs.begin();
		Nes::Address lastAddr = *blockStartAddrs.rbegin();
		return firstAddr >= start && lastAddr < end;*/

	// 如果是尾调用的话，就把 JMP 当作调用指令来处理
	// 移除所有函数范围外的地址，把这些地址当作函数调用处理
	for (int i = (int)blockStartAddrs.size() - 1; i > 0; --i)
	{
		if (blockStartAddrs[i] > this->subroutine->GetEndAddress())
		{
			this->subroutine->AddCall(blockStartAddrs[i]);
			blockStartAddrs.pop_back();
		}
		else
		{
			break;
		}
	}
	return true;
}

void NesSubroutineParser::ParseBasicBlocks()
{
	NesBasicBlock* lastBlock = nullptr;
	NesBasicBlock* entryBlock = nullptr;
	// 每一个地址创建一个基本块对象
	// 不要在这里设置基本块的后继基本块，因为一个基本块可能以RTS结尾
	// 它后面紧跟的基本块不是它的后继基本块
	for (auto addr : blockStartAddrs)
	{
		NesBasicBlock* block = db.allocator.New<NesBasicBlock>();
		block->SetStartAddress(addr);
		if (lastBlock)
			lastBlock->SetEndAddress(block->GetStartAddress());
		lastBlock = block;
		if (!entryBlock)  // 第一个基本块作为入口基本块
			entryBlock = block;

		subroutine->AddBasicBlock(block);
		db.AddBasicBlock(block);
	}
	lastBlock->SetEndAddress(this->subroutine->GetEndAddress());

	// 设置入口基本块
	if (!blockStartAddrs.empty())
		entryBlock->flag |= BBF_ENTRY;

	for (auto block : this->subroutine->GetBasicBlocks())
	{
		ParseBasicBlockInstructions(block);
	}
}

void NesSubroutineParser::ParseBasicBlockInstructions(NesBasicBlock* block)
{
	Instruction instruction;
	for (auto addr = block->GetStartAddress(); addr < block->GetEndAddress();)
	{
		auto p = db.GetCartridge().GetData(addr);
		instruction.Set(addr, p);
		int bytes = instruction.GetLength();
		p += bytes;
		addr += bytes;
		ParseBasicBlockInstruction(block, instruction, addr);
	}
}

void NesSubroutineParser::ParseBasicBlockInstruction(NesBasicBlock* block, const Instruction& instruction, Nes::Address nextAddr)
{
	BlockInstructionKind kind = blockInstructionMap[instruction.GetOperatorByte()];
	switch (kind)
	{
	case BlockInstructionKind::Call:
		this->subroutine->AddCall(instruction.GetOperandAddress());
		// 继续执行
	case BlockInstructionKind::Normal:
	{
										 // 后面一条指令属于下一个基本块
										 // 那么这条指令就是这个基本块的末尾
										 NesBasicBlock* next = this->subroutine->FindBasicBlock(nextAddr);
										 if (next)
										 {
											 block->nexts[0] = next;
											 next->AddPrev(block);
											 block->flag |= BBF_END_NORMAL;
										 }
										 break;
	}
	case BlockInstructionKind::End:
		block->flag |= BBF_END_RETURN;
		break;
	case BlockInstructionKind::ConditionalJump:
	{
												  Sprintf<> s;
												  // 处理条件为假的情况, 条件为假则下一条指令是新的基本块
												  int length = instruction.GetLength();
												  Nes::Address address = instruction.address + length;
												  NesBasicBlock* next = this->subroutine->FindBasicBlock(address);
												  if (!next)
												  {
													  COUT << s.Format(_T("无法在子程序 %04X 中找到地址为 %04X 的基本块\n"),
														  this->subroutine->GetStartAddress(), address);
													  s.Clear();
												  }
												  else
												  {
													  block->nexts[0] = next;
													  next->AddPrev(block);
												  }
												  // 处理条件为真的情况
												  address += (char)instruction.GetByte();
												  next = this->subroutine->FindBasicBlock(address);
												  if (!next)
												  {
													  COUT << s.Format(_T("无法在子程序 %04X 中找到地址为 %04X 的基本块\n"),
														  this->subroutine->GetStartAddress(), address);
												  }
												  else
												  {
													  block->nexts[1] = next;
													  next->AddPrev(block);
												  }
												  SetBasickBlockJumpFlag(block, true, address);
												 break;
	}
	case BlockInstructionKind::UnconditionalJump:
	{
													auto& entry = instruction.GetEntry();
													Nes::Address jumpAddr;
													if (entry.addrMode == AddrMode::Absolute)
														jumpAddr = instruction.GetOperandAddress();
													else
													{
														TCHAR buffer[128];
														_stprintf_s(buffer, 128, _T("地址为 %04X 的无条件跳转指令的非绝对寻址模式未实现"), instruction.address);
														throw Exception(buffer);
													}
													NesBasicBlock* next = this->subroutine->FindBasicBlock(jumpAddr);
													if (!next)
													{
														/*printf("无法在子程序 %04X 中找到地址为 %04X 的基本块\n",
															this->subroutine->GetStartAddress(), jumpAddr);*/
														// 说明是跳转到子程序外的尾调用 JMP，忽略
													}
													else
													{
														block->nexts[0] = next;
														next->AddPrev(block);
													}
													SetBasickBlockJumpFlag(block, false, jumpAddr);
													break;
	}
	}
}

void NesSubroutineParser::BindBlock(NesBasicBlock* prev, Nes::Address nextAddr)
{
	if (!prev)
		return;

	NesBasicBlock* next = this->subroutine->FindBasicBlock(nextAddr);
	if (!next)
	{
		Sprintf<> s;
		COUT << s.Format(_T("无法在子程序 %04X 中找到地址为 %04X 的基本块\n"),
			this->subroutine->GetStartAddress(), nextAddr);
		return;
	}
}

void NesSubroutineParser::SetBasickBlockJumpFlag(NesBasicBlock* block, bool isCond, Nes::Address jumpAddr)
{
	if (isCond)
		block->flag |= BBF_END_COND;
	else
		block->flag |= BBF_END_UNCOND;
	if (jumpAddr <= block->GetStartAddress())
	{
		block->flag |= BBF_JUMP_BEFOER;
		if (jumpAddr == block->GetStartAddress())
			block->flag |= BBF_JUMP_SELF;
	}
}
