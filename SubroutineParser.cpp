#include "stdafx.h"
#include "SubroutineParser.h"
#include "NesDataBase.h"
using namespace NesDB;
using namespace Nes;

NesDB::SubroutineParser::SubroutineParser(NesDataBase& db_):
    db(db_)
{
}

NesSubroutine* SubroutineParser::Parse(Nes::Address start)
{
	if (start == 0x8E19)
	{
		int a = 0;
	}
	subroutine = db.FindSubroutine(start);
	if (subroutine)
		return subroutine;

	Reset();

	Instruction instruction;
	Address end = 0x10000;
	Address jumpAddr, current = 0;
	int bytes;

	std::vector<Address> queue;
	std::unordered_set<NesBasicBlock*> blocks;  // 需要设置前驱的基本块列表
	subroutine = db.GetAllocator().New <NesSubroutine>();
	subroutine->SetStartAddress(start);

	queue.push_back(start);
	while (!queue.empty())
	{
		current = queue.back();
		queue.pop_back();

		if (subroutine->GetBasicBlock(current))
			continue;

		// 在数据库中查找包含当前地址的基本块或下一个基本块
		auto bb = db.GetBasicBlockOrNext(current);
		if (bb)
		{
			auto block = bb;
			if (current > bb->GetStartAddress())
			{
				// 地址在基本块中间，需要分割为两个基本块
				block = db.GetAllocator().New<NesBasicBlock>();
				bb->Split(current, block);
				db.AddBasicBlock(block);
				bb->SetEndFlag(BBF_END_NORMAL);
				subroutine->AddBasicBlock(block);
				// 将所有后继加入到函数的基本块中
				for (auto succ : block->GetSuccs())
					queue.push_back(succ);
				// 如果bb是需要回填前驱的基本块，那么改为block需要回填
				if (blocks.find(bb) != blocks.end())
				{
					blocks.erase(bb);
					blocks.insert(block);
				}
			/*	COUT << s.Format(_T("\n拆分基本块 %04X - %04X - %04X\n"),
					bb->GetStartAddress(), block->GetStartAddress(), block->GetEndAddress());
				bb->Dump();
				block->Dump();*/
				
				continue;
			}
			else if (current == bb->GetStartAddress())
			{
				// 该地址的基本块已经存在
				subroutine->AddBasicBlock(block);
				// 将所有后继加入到函数的基本块中
				for (auto succ : block->GetSuccs())
					queue.push_back(succ);
				continue;
			}
			// 当前基本块不能超过下一个基本块的开始地址
			end = bb->GetStartAddress();
		}

		NesBasicBlock* block = db.GetAllocator().New<NesBasicBlock>();
		block->SetStartAddress(current);
		blocks.insert(block);

		bool blockEnded = false;
		const uint8_t* p = db.GetCartridge().GetData(current);
		while (!blockEnded)
		{
			instruction.Set(current, p);  // 构造指令对象
			const auto& entry = instruction.GetEntry();
			bytes = entry.length;
			current += bytes;  // 计算下一条指令的地址
			p += bytes;

			switch (entry.opcode)
			{
			case Opcode::Jmp:
				blockEnded = true;
				if (entry.addrMode == AddrMode::Absolute)
				{
					jumpAddr = instruction.GetOperandAddress();
					//if (jumpAddr == instruction.GetAddress())
					//{
					//	// while (1); 的情况，当作函数结束处理
					//	break;
					//}
					queue.push_back(jumpAddr);
					block->AddSucc(jumpAddr);
					block->SetEndFlag(BBF_END_UNCOND);
					break;
				}
				throw Exception(_T("未实现"));
				break;
			case Opcode::Bpl:
			case Opcode::Bmi:
			case Opcode::Bne:
			case Opcode::Beq:
			case Opcode::Bcc:
			case Opcode::Bcs:
			case Opcode::Bvc:
			case Opcode::Bvs:
				blockEnded = true;
				jumpAddr = instruction.address + entry.length;
				queue.push_back(jumpAddr);
				block->AddSucc(jumpAddr);
				// 处理条件为真的情况
				jumpAddr += (char)instruction.GetByte();
				queue.push_back(jumpAddr);
				block->AddSucc(jumpAddr);
				block->SetEndFlag(BBF_END_COND);
				break;
			case Opcode::Rts:
			case Opcode::Rti:
				blockEnded = true;
				block->SetEndFlag(BBF_END_RETURN);
				break;
			case Opcode::Jsr:
				subroutine->AddCall(instruction.GetOperandAddress());
				break;
			}
			if (current >= end)
			{
				// 接触到下一个基本块了
				queue.push_back(jumpAddr);
				block->AddSucc(jumpAddr);
				break;
			}
		}

		block->SetEndAddress(current);
		OnEndBasicBlock(block, instruction);
	}

	// 上面的代码只能添加基本块的后继，还添加前驱
	for (auto block : blocks)
	{
		for (auto succ : block->GetSuccs())
		{
			auto succBlock = db.GetBasicBlock(succ);
			succBlock->AddPred(block->GetStartAddress());
		}
	}

	subroutine->SetEndAddress(current);  // 只有指令是连续存放的时候才有意义
	db.AddSubroutine(subroutine);
	if (subroutine->GetStartAddress() == 0x8E19)
	{
		for (auto block : subroutine->GetBasicBlocks())
		{
			block->Dump();
		}
		int a = 0;
	}
    return subroutine;
}

void SubroutineParser::Reset()
{
	subroutine = nullptr;
}

void NesDB::SubroutineParser::OnEndBasicBlock(NesBasicBlock* block, const Instruction& instruction)
{
	subroutine->AddBasicBlock(block);
	db.AddBasicBlock(block);
}
