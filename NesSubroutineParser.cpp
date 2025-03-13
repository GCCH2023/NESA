#include "stdafx.h"
#include "NesSubroutineParser.h"
#include "Instruction.h"
#include "NesDataBase.h"
using namespace Nes;

// ���ڽ��л����黮�ֵ�ָ�����
enum class BlockInstructionKind
{
	Normal = 0,  // ƽ��ָ��
	End,  // �����������ָ��
	UnconditionalJump,  // ��������תָ��
	ConditionalJump,  // ������תָ��
	Call, // �ӳ������ָ��
};

bool isInitedBlockInstructionMap = false;
// ָ��ĵ�һ���ֽ�������ָ������
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

// �����ӳ����ָ���������ŵ�
// ��һ�������ӳ���ͷ��ʼ��һֱȡָ��ֱ����������ָ��
// ��ȡָ��Ĺ����У�����תĿ���ַ���������鿪ʼ��ַ���뵽�б���
// ����������ָ��ʱ���ж���һ��ָ���Ƿ��ǻ����鿪ʼָ������
// �ͼ�������������������ӳ���ķ���
// �ڶ�������������������
NesSubroutine* NesSubroutineParser::Parse(Nes::Address address)
{
	blockStartAddrs.clear();
	blockStartAddrs.push_back(address);  // �ʼ��ʱ��ֻ�к�����ʼ��ַ

	Instruction instruction;
	// �ӿ����л�ȡ������ͷָ��
	const uint8_t* p = db.GetCartridge().GetData(address);
	Nes::Address current = address;  // ��ǰ������ָ���ַ
	int bytes;
	// ���Ȼ��ֻ�����
	for (;; p += bytes)
	{
		instruction.Set(current, p);  // ����ָ�����
		bytes = instruction.GetLength();
		current += bytes;  // ������һ��ָ��ĵ�ַ
		if (!ParseInstruction(instruction))  // ����ָ��
		{
			// ��������ָ���ʱ�������һ��ָ���ǻ����鿪ʼ�Ļ�����Ȼ��������
			if (!IsBlockStartAddress(current))
				break;
		}
	}

	this->subroutine = db.allocator.New<NesSubroutine>(address, current);

	if (!CheckBlocksAddress(address, current))
	{
		TCHAR buffer[64];
		_stprintf_s(buffer, _T("�ӳ��� %04X, �����鳬��������Χ"), address);
		throw Exception(buffer);
	}

	// ���ӻ����飬���ɿ�����ͼ
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
	COUT << s.Format(_T("���� %04X �Ľ�����ַΪ: %04X\n"), subroutine->GetStartAddress(), subroutine->GetEndAddress());
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
		return false;  // �ӳ������ָ��
	case BlockInstructionKind::ConditionalJump:
	{
													// ��������Ϊ�ٵ����, ����Ϊ������һ��ָ�����µĻ�����
													int length = instruction.GetLength();
													Nes::Address address = instruction.address + length;
													AddBasicBlockStartAddress(address);
													// ��������Ϊ������
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
													  _stprintf_s(buffer, 128, _T("��ַΪ %04X ����������תָ��ķǾ���Ѱַģʽδʵ��"), instruction.address);
													  throw Exception(buffer);
												  }

												  AddBasicBlockStartAddress(jumpAddr);
												  return false;  // ��������ת�����ָ��ᱻִ�е��ˣ����Բ��ؼ�������
	}
	}
	return true;
}

void NesSubroutineParser::AddBasicBlockStartAddress(Nes::Address address)
{
	// ʹ�� std::lower_bound ���Ҳ���λ��
	auto it = std::lower_bound(blockStartAddrs.begin(), blockStartAddrs.end(), address);

	// ����λ���Ƿ��Ѿ�������ͬ��ֵ
	if (it == blockStartAddrs.end() || *it != address)
		// �����������ͬ��ֵ���������ֵ
		blockStartAddrs.insert(it, address);
}

bool NesSubroutineParser::IsBlockStartAddress(Nes::Address address)
{
	// ʹ�� std::lower_bound ���Ҳ���λ��
	auto it = std::lower_bound(blockStartAddrs.begin(), blockStartAddrs.end(), address);
	return it != blockStartAddrs.end() && *it == address;
}

// ������л������Ƿ�λ���ӳ����ַ��Χ��
// !! �����п����ӳ�����JMP��β������һ����Զ�ĵ�ַ��Ҳ����β����
// ���ԣ�������������
bool NesSubroutineParser::CheckBlocksAddress(Nes::Address start, Nes::Address end)
{
	/*if (blockStartAddrs.empty())
		return true;
		Nes::Address firstAddr = *blockStartAddrs.begin();
		Nes::Address lastAddr = *blockStartAddrs.rbegin();
		return firstAddr >= start && lastAddr < end;*/

	// �����β���õĻ����Ͱ� JMP ��������ָ��������
	// �Ƴ����к�����Χ��ĵ�ַ������Щ��ַ�����������ô���
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
	// ÿһ����ַ����һ�����������
	// ��Ҫ���������û�����ĺ�̻����飬��Ϊһ�������������RTS��β
	// ����������Ļ����鲻�����ĺ�̻�����
	for (auto addr : blockStartAddrs)
	{
		NesBasicBlock* block = db.allocator.New<NesBasicBlock>();
		block->SetStartAddress(addr);
		if (lastBlock)
			lastBlock->SetEndAddress(block->GetStartAddress());
		lastBlock = block;
		if (!entryBlock)  // ��һ����������Ϊ��ڻ�����
			entryBlock = block;

		subroutine->AddBasicBlock(block);
		db.AddBasicBlock(block);
	}
	lastBlock->SetEndAddress(this->subroutine->GetEndAddress());

	// ������ڻ�����
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
		// ����ִ��
	case BlockInstructionKind::Normal:
	{
										 // ����һ��ָ��������һ��������
										 // ��ô����ָ���������������ĩβ
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
												  // ��������Ϊ�ٵ����, ����Ϊ������һ��ָ�����µĻ�����
												  int length = instruction.GetLength();
												  Nes::Address address = instruction.address + length;
												  NesBasicBlock* next = this->subroutine->FindBasicBlock(address);
												  if (!next)
												  {
													  COUT << s.Format(_T("�޷����ӳ��� %04X ���ҵ���ַΪ %04X �Ļ�����\n"),
														  this->subroutine->GetStartAddress(), address);
													  s.Clear();
												  }
												  else
												  {
													  block->nexts[0] = next;
													  next->AddPrev(block);
												  }
												  // ��������Ϊ������
												  address += (char)instruction.GetByte();
												  next = this->subroutine->FindBasicBlock(address);
												  if (!next)
												  {
													  COUT << s.Format(_T("�޷����ӳ��� %04X ���ҵ���ַΪ %04X �Ļ�����\n"),
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
														_stprintf_s(buffer, 128, _T("��ַΪ %04X ����������תָ��ķǾ���Ѱַģʽδʵ��"), instruction.address);
														throw Exception(buffer);
													}
													NesBasicBlock* next = this->subroutine->FindBasicBlock(jumpAddr);
													if (!next)
													{
														/*printf("�޷����ӳ��� %04X ���ҵ���ַΪ %04X �Ļ�����\n",
															this->subroutine->GetStartAddress(), jumpAddr);*/
														// ˵������ת���ӳ������β���� JMP������
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
		COUT << s.Format(_T("�޷����ӳ��� %04X ���ҵ���ַΪ %04X �Ļ�����\n"),
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
