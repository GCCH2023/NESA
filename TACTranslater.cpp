#include "stdafx.h"
#include "TACTranslater.h"
using namespace Nes;

TACTranslater::TACTranslater(NesDataBase& db_, Allocator& allocator_) :
db(db_),
allocator(allocator_),
tacSub(nullptr)
{

}

TACTranslater::~TACTranslater()
{
}

TACSubroutine* TACTranslater::Translate(NesSubroutine* subroutine)
{
	if (!subroutine)
		return nullptr;
	this->nesSub = subroutine;

	std::unordered_map<NesBasicBlock*, TACBasicBlock*> blockMap;
	this->tacSub = allocator.New<TACSubroutine>(subroutine->GetStartAddress(), subroutine->GetEndAddress());
	for (auto block : subroutine->GetBasicBlocks())
	{
		auto tacBlock = TranslateBasickBlock(block);
		this->tacSub->AddBasicBlock(tacBlock);
		blockMap[block] = tacBlock;
	}

	// �ع�������ı�
	for (auto block : subroutine->GetBasicBlocks())
	{
		auto tacBlock = blockMap[block];

		for (auto prev : block->prevs)
			tacBlock->prevs.push_back(blockMap[prev]);
		if (block->nexts[0])
		{
			tacBlock->nexts.push_back(blockMap[block->nexts[0]]);
			if (block->nexts[1])
				tacBlock->nexts.push_back(blockMap[block->nexts[1]]);
		}
		tacBlock->flag = block->flag;
	}
	return this->tacSub;
}

TACBasicBlock* TACTranslater::TranslateBasickBlock(NesBasicBlock* block)
{
	TCHAR buffer[256];
	// ���ȹ���ָ����У����������ǰ����
	std::vector<Instruction> instructions(256);
	instructions.clear();
	db.GetInstructions(instructions, block->GetStartAddress(), block->GetEndAddress());
	
	this->tacBlock = allocator.New<TACBasicBlock>(block->GetStartAddress(), block->GetEndAddress());

	// ����ָ�������ַ��
	TAC* tac = nullptr, *lastTuple = nullptr;
	Instruction* last = nullptr;
	for (size_t index = 0; index < instructions.size();++index)
	{
		auto& i = instructions[index];
		const OpcodeEntry& entry = GetOpcodeEntry(i.GetOperatorByte());
		switch (entry.opcode)
		{
		case Nes::Opcode::None:
			continue;  // ����
		case Nes::Opcode::Brk:
			tac = allocator.New<TAC>(TACOperator::BREAK);
			break;
		case Nes::Opcode::Ora:
			tac = allocator.New<TAC>(TACOperator::BOR, RegisterA, RegisterA, GetOperand(i));
			break;
		case Nes::Opcode::Nop:
			tac = allocator.New<TAC>(TACOperator::NOP);
			break;
		case Nes::Opcode::Asl:
			tac = allocator.New<TAC>(TACOperator::SHL, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Lsr:
			tac = allocator.New<TAC>(TACOperator::SHR, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Bpl:
			// ��Ҫ��ǰ���ָ��ϲ�
			if ((last->GetEntry().kind & OpKind::Write_N) == 0)
			{
				_stprintf_s(buffer, _T("��ַ %04X ����ʧ�ܣ���תָ�����һ��ָ��û��д����Ҫ�ı�־λ"), i.address);
				throw Exception(buffer);
			}
			// �����µ�����ַ�룬ǰһ��ָ��Ľ����Ϊ��һ���Ĳ�����
			tac = allocator.New<TAC>(TACOperator::IFGEQ, GetOperand(i), lastTuple->z, TACOperand(0));
			break;
		case Nes::Opcode::Bmi:
			// ��Ҫ��ǰ���ָ��ϲ�
			if ((last->GetEntry().kind & OpKind::Write_N) == 0)
			{
				_stprintf_s(buffer, _T("��ַ %04X ����ʧ�ܣ���תָ�����һ��ָ��û��д����Ҫ�ı�־λ"), i.address);
				throw Exception(buffer);
			}
			// �����µ�����ַ�룬ǰһ��ָ��Ľ����Ϊ��һ���Ĳ�����
			tac = allocator.New<TAC>(TACOperator::IFLESS, GetOperand(i), lastTuple->z, TACOperand(0));
			break;
		case Nes::Opcode::Bne:
			// ��Ҫ��ǰ���ָ��ϲ�
			if ((last->GetEntry().kind & OpKind::Write_Z) == 0)
			{
				_stprintf_s(buffer, _T("��ַ %04X ����ʧ�ܣ���תָ�����һ��ָ��û��д����Ҫ�ı�־λ"), i.address);
				throw Exception(buffer);
			}
			// �����µ�����ַ�룬ǰһ��ָ��Ľ����Ϊ��һ���Ĳ�����
			tac = allocator.New<TAC>(TACOperator::IFNEQ, GetOperand(i), lastTuple->z, TACOperand(0));
			break;
		case Nes::Opcode::Beq:
			// ��Ҫ��ǰ���ָ��ϲ�
			if ((last->GetEntry().kind & OpKind::Write_Z) == 0)
			{
				_stprintf_s(buffer, _T("��ַ %04X ����ʧ�ܣ���תָ�����һ��ָ��û��д����Ҫ�ı�־λ"), i.address);
				throw Exception(buffer);
			}
			// �����µ�����ַ�룬ǰһ��ָ��Ľ����Ϊ��һ���Ĳ�����
			tac = allocator.New<TAC>(TACOperator::IFEQ, GetOperand(i), lastTuple->z, TACOperand(0));
			break;
		case Nes::Opcode::Bcc:
			// ��Ҫ��ǰ���ָ��ϲ�
			if ((last->GetEntry().kind & OpKind::Write_C) == 0)
			{
				_stprintf_s(buffer, _T("��ַ %04X ����ʧ�ܣ���תָ�����һ��ָ��û��д����Ҫ�ı�־λ"), i.address);
				throw Exception(buffer);
			}
			// �����µ�����ַ�룬ǰһ��ָ��Ľ����Ϊ��һ���Ĳ�����
			tac = allocator.New<TAC>(TACOperator::IFEQ, GetOperand(i), lastTuple->z, TACOperand(0));
			break;
		case Nes::Opcode::Bcs:
			// ��Ҫ��ǰ���ָ��ϲ�
			if ((last->GetEntry().kind & OpKind::Write_C) == 0)
			{
				_stprintf_s(buffer, _T("��ַ %04X ����ʧ�ܣ���תָ�����һ��ָ��û��д����Ҫ�ı�־λ"), i.address);
				throw Exception(buffer);
			}
			// �����µ�����ַ�룬ǰһ��ָ��Ľ����Ϊ��һ���Ĳ�����
			tac = allocator.New<TAC>(TACOperator::IFNEQ, GetOperand(i), lastTuple->z, TACOperand(0));
			break;
		case Nes::Opcode::Cli:
			tac = allocator.New<TAC>(TACOperator::BAND, RegisterP, RegisterP, (uint8_t)~Flags::I);
			break;
		case Nes::Opcode::Sei:
			tac = allocator.New<TAC>(TACOperator::BOR, RegisterP, RegisterP, Flags::I);
			break;
		case Nes::Opcode::Clc:
			tac = allocator.New<TAC>(TACOperator::BAND, RegisterP, RegisterP, (uint8_t)~Flags::C);
			break;
		case Nes::Opcode::Sec:
			tac = allocator.New<TAC>(TACOperator::BOR, RegisterP, RegisterP, Flags::C);
			break;
		case Nes::Opcode::Clv:
			tac = allocator.New<TAC>(TACOperator::BAND, RegisterP, RegisterP, (uint8_t)~Flags::V);
			break;
		case Nes::Opcode::Cld:
			tac = allocator.New<TAC>(TACOperator::BAND, RegisterP, RegisterP, (uint8_t)~Flags::D);
			break;
		case Nes::Opcode::Sed:
			tac = allocator.New<TAC>(TACOperator::BOR, RegisterP, RegisterP, Flags::D);
			break;
		case Nes::Opcode::Jsr:
		{
								 // 
								 tac = allocator.New<TAC>(TACOperator::CALL, GetOperand(i));
								 tac->z.SetKind(TACOperand::ADDRESS);
								 auto sub = db.FindSubroutine(tac->z.GetValue());
								 if (sub && sub->flag & SUBF_PARAM)
								 {
									 if (sub->flag & SUBF_PARAM_A)
									 {
										 AddTAC(allocator.New<TAC>(TACOperator::ARG, 0, RegisterA));
									 }
									 if (sub->flag & SUBF_PARAM_X)
									 {
										 AddTAC(allocator.New<TAC>(TACOperator::ARG, 0, RegisterX));
									 }
									 if (sub->flag & SUBF_PARAM_Y)
									 {
										 AddTAC(allocator.New<TAC>(TACOperator::ARG, 0, RegisterY));
									 }
								 }
								 break;
		}
		case Nes::Opcode::And:
			tac = allocator.New<TAC>(TACOperator::BAND, RegisterA, RegisterA, GetOperand(i));
			break;
		case Nes::Opcode::Bit:
			tac = allocator.New<TAC>(TACOperator::BIT, TACOperand(0), RegisterA, GetOperand(i));
			break;
		case Nes::Opcode::Eor:
			tac = allocator.New<TAC>(TACOperator::XOR, RegisterA, RegisterA, GetOperand(i));
			break;
		case Nes::Opcode::Pha:
			tac = allocator.New<TAC>(TACOperator::PUSH);
			tac->x = RegisterA;
			break;
		case Nes::Opcode::Pla:
			tac = allocator.New<TAC>(TACOperator::POP, RegisterA);
			break;
		case Nes::Opcode::Php:
			tac = allocator.New<TAC>(TACOperator::PUSH);
			tac->x = RegisterP;
			break;
		case Nes::Opcode::Plp:
			tac = allocator.New<TAC>(TACOperator::POP, RegisterP);
			break;
		case Nes::Opcode::Jmp:
		{
								 // ��������JMP��ʾ��β����
								 if (entry.addrMode == AddrMode::Absolute)
								 {
									 auto jumpAddr = i.GetOperandAddress();
									 if (jumpAddr < this->nesSub->GetStartAddress() ||
										 jumpAddr >= this->nesSub->GetEndAddress())
									 {
										 // ��Ϊ��β����
										 tac = allocator.New<TAC>(TACOperator::CALL, GetOperand(i));
										 tac->z.SetKind(TACOperand::ADDRESS);
										 break;
									 }
								 }
								 tac = allocator.New<TAC>(TACOperator::GOTO, GetOperand(i));
								 tac->z.SetKind(TACOperand::ADDRESS);
								 break;
		}
		case Nes::Opcode::Rts:
		case Nes::Opcode::Rti:
			tac = allocator.New<TAC>(TACOperator::RETURN);
			break;
		case Nes::Opcode::Rol:
			tac = allocator.New<TAC>(TACOperator::ROL, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Ror:
			tac = allocator.New<TAC>(TACOperator::ROR, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Sta:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, GetOperand(i), RegisterA);
			break;
		case Nes::Opcode::Stx:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, GetOperand(i), RegisterX);
			break;
		case Nes::Opcode::Sty:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, GetOperand(i), RegisterY);
			break;
		case Nes::Opcode::Txa:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterA, RegisterX);
			break;
		case Nes::Opcode::Tax:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterX, RegisterA);
			break;
		case Nes::Opcode::Tay:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterY, RegisterA);
			break;
		case Nes::Opcode::Tya:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterA, RegisterY);
			break;
		case Nes::Opcode::Txs:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterSP, RegisterX);
			break;
		case Nes::Opcode::Tsx:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterX, RegisterSP);
			break;
		case Nes::Opcode::Lda:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterA, GetOperand(i));
			break;
		case Nes::Opcode::Ldx:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterX, GetOperand(i));
			break;
		case Nes::Opcode::Ldy:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterY, GetOperand(i));
			break;
		case Nes::Opcode::Cmp:
			tac = TranslateCmp(i, instructions[index + 1], RegisterA);
			++index;
			break;
		case Nes::Opcode::Cpx:
			tac = TranslateCmp(i, instructions[index + 1], RegisterX);
			++index;
			break;
		case Nes::Opcode::Cpy:
			tac = TranslateCmp(i, instructions[index + 1], RegisterY);
			++index;
			break;
		case Nes::Opcode::Inc:
			tac = allocator.New<TAC>(TACOperator::ADD, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Dec:
			tac = allocator.New<TAC>(TACOperator::SUB, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Inx:
			tac = allocator.New<TAC>(TACOperator::ADD, RegisterX, RegisterX, TACOperand(1));
			break;
		case Nes::Opcode::Iny:
			tac = allocator.New<TAC>(TACOperator::ADD, RegisterY, RegisterY, TACOperand(1));
			break;
		case Nes::Opcode::Dex:
			tac = allocator.New<TAC>(TACOperator::SUB, RegisterX, RegisterX, TACOperand(1));
			break;
		case Nes::Opcode::Dey:
			tac = allocator.New<TAC>(TACOperator::SUB, RegisterY, RegisterY, TACOperand(1));
			break;
		default:
		{
				   TCHAR buffer[64];
				   _stprintf_s(buffer, _T("NESָ��ת����ַ��: δʵ�ֵ�NESָ�� %s"), ToString(entry.opcode));
				   throw Exception(buffer);
		}
		}
		if (tac)
		{
			tac->address = i.GetAddress();
			AddTAC(tac);
			lastTuple = tac;
			tac = nullptr;
		}
		last = &i;
	}
	return tacBlock;
}

TACOperand TACTranslater::GetOperand(const Instruction& instruction)
{
	const OpcodeEntry& entry = instruction.GetEntry();
	switch (entry.addrMode)
	{
	case AddrMode::Accumulator:
		return RegisterA;
	case AddrMode::Immediate:
		return TACOperand(instruction.GetByte());
	case AddrMode::Absolute:
		return TACOperand(TACOperand::MEMORY | instruction.GetOperandAddress());
	case AddrMode::AbsoluteX:
	{
								// ��Ҫ�������һ������ַ�����ڼ����ַ
								int temp = this->tacSub->NewTemp(2);
								TACOperand result(TACOperand::TEMP | temp);  // ����һ����ʱ���������������ַ
								TAC* tuple = allocator.New<TAC>(TACOperator::ADD, result, RegisterX, instruction.GetOperandAddress());
								tuple->address = instruction.GetAddress();
								AddTAC(tuple);
								// Ȼ�󷵻���ʱ������Ϊ��ַ
								result.SetKind(TACOperand::MEMORY);
								return result;
	}
	case AddrMode::AbsoluteY:
	{
								// ��Ҫ�������һ������ַ�����ڼ����ַ
								int temp = this->tacSub->NewTemp(2);
								TACOperand result(TACOperand::TEMP | temp);  // ����һ����ʱ���������������ַ
								TAC* tuple = allocator.New<TAC>(TACOperator::ADD, result, RegisterX, instruction.GetOperandAddress());
								tuple->address = instruction.GetAddress();
								AddTAC(tuple);
								// Ȼ�󷵻���ʱ����
								result.SetKind(TACOperand::MEMORY);
								return result;
	}
	case AddrMode::Relative:
		return TACOperand(TACOperand::ADDRESS | instruction.GetConditionalJumpAddress());
	case AddrMode::ZeroPage:
		return TACOperand(TACOperand::MEMORY | instruction.GetByte());
	case AddrMode::IndirectY:
	{
								// [Y + [zp]]
								// ��Ҫ�������һ������ַ�����ڼ����ַ
								int temp = this->tacSub->NewTemp(2);
								TACOperand result(TACOperand::TEMP | temp);  // ����һ����ʱ���������������ַ
								TACOperand zeroPageAddr(TACOperand::MEMORY | instruction.GetByte());  // ����ҳָ��2�ֽڵ�Ԫȡ����ַ
								TAC* tuple = allocator.New<TAC>(TACOperator::ADD, result, RegisterY, zeroPageAddr);
								tuple->address = instruction.GetAddress();
								AddTAC(tuple);
								// Ȼ�󷵻���ʱ������Ϊ��ַ
								result.SetKind(TACOperand::MEMORY);
								return result;
	}
	default:
	{
			   TCHAR buffer[64];
			   _stprintf_s(buffer, _T("%04X ��ȡָ��Ĳ�����δʵ�ֵ�Ѱַģʽ %s"), instruction.GetAddress(),
				   Nes::ToString(entry.addrMode));
			   throw Exception(buffer);
	}
	}
	return TACOperand();
}

TAC* TACTranslater::TranslateCmp(const Instruction& instruction, const Instruction& next, TACOperand reg)
{
	TCHAR buffer[128];
	// ������һ����ʱ����ַ��ռλ
	auto tuple = allocator.New<TAC>(TACOperator::NOP, TACOperand(), reg, GetOperand(instruction));
	// Ҫ����һ��ָ�������������תָ��
	auto& entry = next.GetEntry();
	if ((entry.kind & OpKind::ConditionalJump) == 0)
	{
		_stprintf_s(buffer, _T("��ַ %04X ����ʧ�ܣ�CMPָ�����һ��ָ�����תָ��"), instruction.address);
		throw Exception(buffer);
	}
	switch (entry.opcode)
	{
	case Opcode::Bcs:
		tuple->op = TACOperator::IFGEQ;
		break;
	case Opcode::Beq:
		tuple->op = TACOperator::IFEQ;
	case Opcode::Bne:
		tuple->op = TACOperator::IFNEQ;
		break;
	case Opcode::Bcc:
		tuple->op = TACOperator::IFLESS;
		break;
	//case Opcode::Bvc:
	//case Opcode::Bvs:
	default:
		throw "δʵ��";
	}
	tuple->z = TACOperand(TACOperand::ADDRESS | next.GetConditionalJumpAddress());
	return tuple;
}

void TACTranslater::AddTAC(TAC* tac)
{
	this->tacSub->AddTAC(tac);
	this->tacBlock->AddTAC(tac);
}
