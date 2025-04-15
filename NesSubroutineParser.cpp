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
void NesSubroutineParser::Parse(NesSubroutine* subroutine)
{
	Reset();

	this->subroutine = subroutine;
	uint32_t start = subroutine->GetStartAddress();
	uint32_t end = subroutine->GetEndAddress();
	blockStartAddrs.push_back(start);  // 最开始的时候只有函数开始地址

	Instruction instruction;
	// 从卡带中获取函数开头指针
	const uint8_t* p = db.GetCartridge().GetData(start);
	Nes::Address current = start;  // 当前分析的指令地址
	int bytes;

	// 首先划分基本块
	for (; current < end; p += bytes)
	{
		instruction.Set(current, p);  // 构造指令对象
		bytes = instruction.GetLength();
		current += bytes;  // 计算下一条指令的地址

		if (!ParseInstruction(instruction))  // 分析指令
		{
			// 遇到结束指令的时候，如果下一条指令是基本块开始的话，仍然继续分析
			if (!IsBlockStartAddress(current))
			{
				if (instruction.GetOpcode() == Opcode::None)
				{
					// 非法指令不计入基本块开始，不包含在函数中
					current -= bytes;
					blockStartAddrs.erase(std::remove(blockStartAddrs.begin(), blockStartAddrs.end(), current), blockStartAddrs.end());
				}
				break;
			}
		}
	}

	subroutine->SetEndAddress(current);
	if (!CheckBlocksAddress(start, current))
	{
		TCHAR buffer[64];
		_stprintf_s(buffer, _T("子程序 %04X, 基本块超出函数范围"), start);
		throw Exception(buffer);
	}

	// 连接基本块，构成控制流图
	ParseBasicBlocks();

	// 判断这个函数是否内联在其他函数中
	if (this->isInline || db.GetSubroutine(start) != nullptr)
		subroutine->SetInline(true);
}

NesSubroutine* NesSubroutineParser::Parse(uint32_t start, uint32_t end)
{
	auto sub = db.allocator.New<NesSubroutine>(start, end);
	Parse(sub);
	return sub;
}

void NesSubroutineParser::Reset()
{
	blockStartAddrs.clear();
	this->subroutine = nullptr;
	this->calls.clear();
	isInline = false;
}


bool NesSubroutineParser::ParseInstruction(const Instruction& instruction)
{
	BlockInstructionKind kind = blockInstructionMap[instruction.GetOperatorByte()];
	switch (kind)
	{
	case BlockInstructionKind::Normal:
		return true;
	case BlockInstructionKind::Call:
		this->subroutine->AddCall(instruction.GetOperandAddress());
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
													{
														jumpAddr = instruction.GetOperandAddress();
													}
													else
													{
														// 间接寻址相当于尾函数调用
														return false;
														/*  TCHAR buffer[128];
														  _stprintf_s(buffer, 128, _T("地址为 %04X 的无条件跳转指令的非绝对寻址模式未实现"), instruction.address);
														  throw Exception(buffer);*/
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

	// 移除所有函数范围外的地址，把这些地址当作函数调用处理
	blockStartAddrs.erase(std::remove_if(blockStartAddrs.begin(), blockStartAddrs.end(),
		[this](Nes::Address addr) {
		if (this->subroutine->Contains(addr))
			return false;
		this->subroutine->AddCall(addr);
		return true;
	}), blockStartAddrs.end());
	return true;
}

void NesSubroutineParser::ParseBasicBlocks()
{
	NesBasicBlock* lastBlock = nullptr;
	// 每一个地址创建一个基本块对象
	// 不要在这里设置基本块的后继基本块，因为一个基本块可能以RTS结尾
	// 它后面紧跟的基本块不是它的后继基本块
	for (auto addr : blockStartAddrs)
	{
		if (!this->subroutine->Contains(addr))
			continue;  // 跳转到函数外的地址不计入函数基本块
		NesBasicBlock* block = db.allocator.New<NesBasicBlock>();
		block->SetStartAddress(addr);
		if (lastBlock)
			lastBlock->SetEndAddress(block->GetStartAddress());
		lastBlock = block;

		this->subroutine->AddBasicBlock(block);
		db.AddBasicBlock(block);
	}
	lastBlock->SetEndAddress(this->subroutine->GetEndAddress());

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
		// 继续执行
	case BlockInstructionKind::Normal:
	{
										 // 后面一条指令属于下一个基本块
										 // 那么这条指令就是这个基本块的末尾
										 NesBasicBlock* next = this->subroutine->GetBasicBlock(nextAddr);
										 if (next)
										 {
											 block->AddSucc(nextAddr);
											 next->AddPred(block->GetStartAddress());
											 block->SetEndFlag(BBF_END_NORMAL);
										 }
										 break;
	}
	case BlockInstructionKind::End:
		block->SetEndFlag(BBF_END_RETURN);
		break;
	case BlockInstructionKind::ConditionalJump:
	{
												  Sprintf<> s;
												  // 处理条件为假的情况, 条件为假则下一条指令是新的基本块
												  int length = instruction.GetLength();
												  Nes::Address address = instruction.address + length;
												  if (this->subroutine->Contains(address))
												  {
													  NesBasicBlock* next = this->subroutine->GetBasicBlock(address);
													  if (!next)
													  {
														  COUT << s.Format(_T("无法在子程序 %04X 中找到地址为 %04X 的基本块\n"),
															  this->subroutine->GetStartAddress(), address);
														  s.Clear();
													  }
													  else
													  {
														  block->AddSucc(address);
														  next->AddPred(block->GetStartAddress());
													  }
												  }
												  // 处理条件为真的情况
												  address += (char)instruction.GetByte();
												  if (this->subroutine->Contains(address))
												  {
													  auto next = this->subroutine->GetBasicBlock(address);
													  if (!next)
													  {
														  COUT << s.Format(_T("无法在子程序 %04X 中找到地址为 %04X 的基本块\n"),
															  this->subroutine->GetStartAddress(), address);
													  }
													  else
													  {
														  block->AddSucc(address);
														  next->AddPred(block->GetStartAddress());
													  }
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
														// 间接寻址相当于尾函数调用
														block->SetEndFlag(BBF_END_RETURN);
														break;
														/*	TCHAR buffer[128];
															_stprintf_s(buffer, 128, _T("地址为 %04X 的无条件跳转指令的非绝对寻址模式未实现"), instruction.address);
															throw Exception(buffer);*/
													}
													if (this->subroutine->Contains(jumpAddr))
													{
														NesBasicBlock* next = this->subroutine->GetBasicBlock(jumpAddr);
														if (!next)
														{
															/*printf("无法在子程序 %04X 中找到地址为 %04X 的基本块\n",
																this->subroutine->GetStartAddress(), jumpAddr);*/
															// 说明是跳转到子程序外的尾调用 JMP，忽略
														}
														else
														{
															block->AddSucc(jumpAddr);
															next->AddPred(block->GetStartAddress());
														}
													}
													SetBasickBlockJumpFlag(block, false, jumpAddr);
													break;
	}
	}
}

//void NesSubroutineParser::BindBlock(NesBasicBlock* prev, Nes::Address nextAddr)
//{
//	if (!prev)
//		return;
//
//	NesBasicBlock* next = this->subroutine->FindBasicBlock(nextAddr);
//	if (!next)
//	{
//		Sprintf<> s;
//		COUT << s.Format(_T("无法在子程序 %04X 中找到地址为 %04X 的基本块\n"),
//			this->subroutine->GetStartAddress(), nextAddr);
//		return;
//	}
//}

void NesSubroutineParser::SetBasickBlockJumpFlag(NesBasicBlock* block, bool isCond, Nes::Address jumpAddr)
{
	if (isCond)
		block->SetEndFlag(BBF_END_COND);
	else
		block->SetEndFlag(BBF_END_UNCOND);

	if (jumpAddr <= block->GetStartAddress())
	{
		block->SetJumpFlag(BBF_JUMP_BEFOER);
		if (jumpAddr == block->GetStartAddress())
			block->SetJumpFlag(BBF_JUMP_SELF);
	}
}
