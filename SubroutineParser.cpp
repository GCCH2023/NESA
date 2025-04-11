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
	subroutine = db.FindSubroutine(start);
	if (subroutine)
		return subroutine;

	Reset();

	Instruction instruction;
	Address end = 0x10000;
	Address jumpAddr, current = 0;
	int bytes;

	subroutine = db.GetAllocator().New <NesSubroutine>();
	subroutine->SetStartAddress(start);

	queue.push_back(start);
	while (!queue.empty())
	{
		current = queue.back();
		queue.pop_back();

		if ((end = ParseExist(current)) == 0)
			continue;

		NesBasicBlock* block = db.GetAllocator().New<NesBasicBlock>();
		block->SetStartAddress(current);

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
				block->AddSucc(jumpAddr);
				break;
			}
		}

		block->SetEndAddress(current);
		AddBasicBlock(block);
	}
	//COUT << _T("准备回填\n");
	//for (auto it : blocks)
	//{
	//	it.second->Dump();
	//}
	// 上面的代码只能添加基本块的后继，还需要添加前驱
	// 同时计算子程序的范围，开始地址到最大连续基本块的末尾地址
	current = subroutine->GetStartAddress();
	for (auto it : blocks)
	{
		auto block = it.second;
		if (block->GetStartAddress() == current)
			current = block->GetEndAddress();
		for (auto succ : block->GetSuccs())
		{
			auto succBlock = GetBasicBlockOrNext(succ);
			assert(succBlock->GetStartAddress() == succ);
			succBlock->AddPred(block->GetStartAddress());
		}
		subroutine->AddBasicBlock(block);
	}

	subroutine->SetEndAddress(current);  // 只有指令是连续存放的时候才有意义
	db.AddSubroutine(subroutine);
    return subroutine;
}

void SubroutineParser::Reset()
{
	subroutine = nullptr;
	queue.clear();
	blocks.clear();
}

NesBasicBlock* NesDB::SubroutineParser::GetBasicBlockOrNext(Nes::Address address)
{
	// 查找第一个结束地址大于指定地址的子程序
	auto it = std::lower_bound(blocks.begin(), blocks.end(), address,
		[](const std::pair<Nes::Address, NesBasicBlock*>& p, Nes::Address address) {
			return p.second->GetEndAddress() <= address;
		});
	// 如果该子程序包含指定地址，则返回它，否则返回空
	if (it == blocks.end())
		return nullptr;
	return it->second;
}

void NesDB::SubroutineParser::AddBasicBlock(NesBasicBlock* block)
{
	blocks.insert({ block->GetStartAddress(), block });
}

Nes::Address NesDB::SubroutineParser::ParseExist(Nes::Address address)
{
	// 查找包含当前地址的基本块或下一个基本块
	auto bb = GetBasicBlockOrNext(address);
	if (bb == nullptr)
		return 0x10000;  // NES 地址上限

	auto block = bb;
	if (address > bb->GetStartAddress())  // 地址在基本块中间，需要分割为两个基本块
	{
		block = db.GetAllocator().New<NesBasicBlock>();
		bb->Split(address, block);
		bb->SetEndFlag(BBF_END_NORMAL);
		block->ClearPreds();  // 避免重复添加
		AddBasicBlock(block);
		return 0;
	}
	// 如果存在开始地址为address的基本块就返回0，
	// 否则返回下一个基本块的开始地址
	return address == bb->GetStartAddress() ? 0 : bb->GetStartAddress();
}