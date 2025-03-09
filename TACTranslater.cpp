#include "stdafx.h"
#include "TACTranslater.h"
using namespace Nes;
#include "CDataBase.h"

TACTranslater::TACTranslater(NesDataBase& db_, Allocator& allocator_) :
db(db_),
allocator(allocator_),
tacSub(nullptr),
tacStarts(32)
{
}

TACTranslater::~TACTranslater()
{
}

TACFunction* TACTranslater::Translate(NesSubroutine* subroutine)
{
	if (!subroutine)
		return nullptr;
	Reset();

	this->nesSub = subroutine;

	std::unordered_map<NesBasicBlock*, TACBasicBlock*> blockMap;
	this->tacSub = allocator.New<TACFunction>(subroutine->GetStartAddress(), subroutine->GetEndAddress());
	this->tacSub->flag = subroutine->flag;

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

void TACTranslater::Reset()
{
	nesSub = nullptr;;
	tacSub = nullptr;;
	tacBlock = nullptr;
	tacStarts.clear();
	axy = -1;
}

// index : ������תָ�������
// need : ��Ҫ�ı�־λ
TAC* TACTranslater::TranslateConditionalJump(std::vector<Instruction>& instructions, size_t index, uint32_t need, TACOperator op)
{
	for (int i = (int)index - 1; i >= 0; --i)
	{
		auto& instruction = instructions[i];
		if ((instruction.GetEntry().kind & need) != 0)  // д���˱�Ҫ�ı�־λ
		{
			// ��������ָ���Ӧ������ַ��
			TAC* tac = GetBlockTAC(i);
			// ����� if x ? y goto z ����ʽ
			return allocator.New<TAC>(op, GetOperand(instructions[index]), tac->z, TACOperand(0));
		}
	}
	TCHAR buffer[256];
	_stprintf_s(buffer, _T("��ַ %04X ����ʧ�ܣ���תָ�����ڵĻ�����û��ָ��д����Ҫ�ı�־λ"), instructions[index].address);
	throw Exception(buffer);
}

TAC* TACTranslater::TranslateCall(Nes::Address callAddr, Nes::Address addr)
{
	int argCount = 0;
	TACOperand x(TACOperand::ADDRESS | callAddr);
	auto tac = allocator.New<TAC>(TACOperator::CALL, 0, x, 0);
	// 1. ���Ҫ���õĺ����в�������ô�ȴ��ݲ���
	auto sub = db.FindSubroutine(tac->x.GetValue());
	if (sub && sub->flag & SUBF_PARAM)
	{
		if (sub->flag & SUBF_PARAM_A)
		{
			AddTAC(allocator.New<TAC>(TACOperator::ARG, 0, RegisterA), addr);
			++argCount;
		}
		if (sub->flag & SUBF_PARAM_X)
		{
			AddTAC(allocator.New<TAC>(TACOperator::ARG, 0, RegisterX), addr);
			++argCount;
		}
		if (sub->flag & SUBF_PARAM_Y)
		{
			AddTAC(allocator.New<TAC>(TACOperator::ARG, 0, RegisterY), addr);
			++argCount;
		}
		tac->y.SetValue(argCount);
	}
	// 2. ������õĺ����з���ֵ����ôʹ�÷���ֵ��AXY�Ĵ�����ֵ
	if (sub && (sub->flag & SUBF_RETURN))
	{
		// (1) ������ axy ���շ���ֵ
		tac->z = TACOperand(TACOperand::TEMP | GetAxy());
		// (2) �� axy ���ֶθ�ֵ�� AXY �Ĵ���
		if (sub->flag & SUBF_RETURN_A)
		{
			AddTAC(tac, addr);
			tac = allocator.New<TAC>(TACOperator::ARRAY_GET, RegisterA,
				TACOperand(TACOperand::TEMP | GetAxy()), 0);
		}
		if (sub->flag & SUBF_RETURN_X)
		{
			AddTAC(tac, addr);
			tac = allocator.New<TAC>(TACOperator::ARRAY_GET, RegisterX,
				TACOperand(TACOperand::TEMP | GetAxy()), 1);
		}
		if (sub->flag & SUBF_RETURN_Y)
		{
			AddTAC(tac, addr);
			tac = allocator.New<TAC>(TACOperator::ARRAY_GET, RegisterY,
				TACOperand(TACOperand::TEMP | GetAxy()), 2);
		}
	}
	return tac;
}

TAC* TACTranslater::TranslateReturn(const Instruction& instruction)
{
	// û�з���ֵ�Ļ���ֱ�ӷ���
	if ((this->tacSub->flag & SUBF_RETURN) == 0)
		return allocator.New<TAC>(TACOperator::RETURN);

	// ������ֵ�ֶθ�ֵ����Ҫ������ķ�ʽ����ֵ
	auto returns = this->tacSub->GetReturnFlag();

	if (returns & Nes::NesRegisters::A)
	{
		auto tac = allocator.New<TAC>(TACOperator::ARRAY_SET, RegisterA,
			TACOperand(TACOperand::TEMP | GetAxy()), 0);
		AddTAC(tac, instruction.GetAddress());
	}
	if (returns & Nes::NesRegisters::X)
	{
		auto tac = allocator.New<TAC>(TACOperator::ARRAY_SET, RegisterX,
			TACOperand(TACOperand::TEMP | GetAxy()), 1);
		AddTAC(tac, instruction.GetAddress());
	}
	if (returns & Nes::NesRegisters::Y)
	{
		auto tac = allocator.New<TAC>(TACOperator::ARRAY_SET, RegisterY,
			TACOperand(TACOperand::TEMP | GetAxy()), 2);
		AddTAC(tac, instruction.GetAddress());
	}
	return allocator.New<TAC>(TACOperator::RETURN, 0, TACOperand(TACOperand::TEMP | GetAxy()));
}

int TACTranslater::GetAxy()
{
	if (axy < 0)
		axy = this->tacSub->NewTemp(GetCDB().GetAXYType());
	return axy;
}

TACBasicBlock* TACTranslater::TranslateBasickBlock(NesBasicBlock* block)
{
	// ���ȹ���ָ����У����������ǰ����
	std::vector<Instruction> instructions(256);
	instructions.clear();
	db.GetInstructions(instructions, block->GetStartAddress(), block->GetEndAddress());

	this->tacBlock = allocator.New<TACBasicBlock>(block->GetStartAddress(), block->GetEndAddress());
	this->tacStarts.clear();

	// ����ָ�������ַ��
	TAC* tac = nullptr;
	Instruction* last = nullptr;
	TAC temp;
	for (size_t index = 0; index < instructions.size(); ++index)
	{
		auto& i = instructions[index];
		const OpcodeEntry& entry = GetOpcodeEntry(i.GetOperatorByte());
		this->SaveTACStart();  // ��¼����ָ���Ӧ������ַ�뿪ʼ����
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
			tac = TranslateConditionalJump(instructions, index, OpKind::Write_N, TACOperator::IFGEQ);
			break;
		case Nes::Opcode::Bmi:
			tac = TranslateConditionalJump(instructions, index, OpKind::Write_N, TACOperator::IFLESS);
			break;
		case Nes::Opcode::Bne:
			tac = TranslateConditionalJump(instructions, index, OpKind::Write_Z, TACOperator::IFNEQ);
			break;
		case Nes::Opcode::Beq:
			tac = TranslateConditionalJump(instructions, index, OpKind::Write_Z, TACOperator::IFEQ);
			break;
		case Nes::Opcode::Bcc:
			tac = TranslateConditionalJump(instructions, index, OpKind::Write_C, TACOperator::IFEQ);
			break;
		case Nes::Opcode::Bcs:
			tac = TranslateConditionalJump(instructions, index, OpKind::Write_C, TACOperator::IFNEQ);
			break;
		case Nes::Opcode::Cli:
			tac = allocator.New<TAC>(TACOperator::CLI);
			break;
		case Nes::Opcode::Sei:
			tac = allocator.New<TAC>(TACOperator::SEI);
			break;
		case Nes::Opcode::Clc:
			tac = allocator.New<TAC>(TACOperator::CLC);
			break;
		case Nes::Opcode::Sec:
			tac = allocator.New<TAC>(TACOperator::SEC);
			break;
		case Nes::Opcode::Clv:
			tac = allocator.New<TAC>(TACOperator::CLV);
			break;
		case Nes::Opcode::Cld:
			tac = allocator.New<TAC>(TACOperator::CLD);
			break;
		case Nes::Opcode::Sed:
			tac = allocator.New<TAC>(TACOperator::SED);
			break;
		case Nes::Opcode::Jsr:
			tac = TranslateCall(i.GetOperandAddress(), i.GetAddress());
			break;
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
										 tac = TranslateCall(i.GetOperandAddress(), i.GetAddress());
										 AddTAC(tac, i.GetAddress());
										 tac = allocator.New<TAC>(TACOperator::RETURN);
										 break;
									 }
								 }
								 tac = allocator.New<TAC>(TACOperator::GOTO, GetOperand(i));
								 tac->z.SetKind(TACOperand::ADDRESS);
								 break;
		}
		case Nes::Opcode::Rts:
		case Nes::Opcode::Rti:
			tac = TranslateReturn(i);
			break;
		case Nes::Opcode::Rol:
			tac = allocator.New<TAC>(TACOperator::ROL, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Ror:
			tac = allocator.New<TAC>(TACOperator::ROR, GetOperand(i), GetOperand(i), TACOperand(1));
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
			if (TranslateOperand(temp, i))
				tac = allocator.New<TAC>(TACOperator::ARRAY_GET, RegisterA, temp.x, temp.y);
			else
				tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterA, temp.x);
			break;
		case Nes::Opcode::Ldx:
			if (TranslateOperand(temp, i))
				tac = allocator.New<TAC>(TACOperator::ARRAY_GET, RegisterX, temp.x, temp.y);
			else
				tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterX, temp.x);
			break;
		case Nes::Opcode::Ldy:
			if (TranslateOperand(temp, i))
				tac = allocator.New<TAC>(TACOperator::ARRAY_GET, RegisterY, temp.x, temp.y);
			else
				tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterY, temp.x);
			break;
		case Nes::Opcode::Sta:
			if (TranslateOperand(temp, i))
				tac = allocator.New<TAC>(TACOperator::ARRAY_SET, RegisterA, temp.x, temp.y);
			else
				tac = allocator.New<TAC>(TACOperator::ASSIGN, temp.x, RegisterA);
			break;
		case Nes::Opcode::Stx:
			if (TranslateOperand(temp, i))
				tac = allocator.New<TAC>(TACOperator::ARRAY_SET, RegisterX, temp.x, temp.y);
			else
				tac = allocator.New<TAC>(TACOperator::ASSIGN, temp.x, RegisterX);
			break;
		case Nes::Opcode::Sty:
			if (TranslateOperand(temp, i))
				tac = allocator.New<TAC>(TACOperator::ARRAY_SET, RegisterY, temp.x, temp.y);
			else
				tac = allocator.New<TAC>(TACOperator::ASSIGN, temp.x, RegisterY);
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
			AddTAC(tac, i.GetAddress());
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
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);  // ����һ����ʱ���������������ַ
								TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterX, instruction.GetOperandAddress());
								AddTAC(tac, instruction.GetAddress());
								// Ȼ�󷵻���ʱ������Ϊ��ַ
								result.SetKind(TACOperand::MEMORY);
								return result;
	}
	case AddrMode::AbsoluteY:
	{
								// ��Ҫ�������һ������ַ�����ڼ����ַ
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);  // ����һ����ʱ���������������ַ
								TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterX, instruction.GetOperandAddress());
								AddTAC(tac, instruction.GetAddress());
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
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);  // ����һ����ʱ���������������ַ
								TACOperand zeroPageAddr(TACOperand::MEMORY | instruction.GetByte());  // ����ҳָ��2�ֽڵ�Ԫȡ����ַ
								TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterY, zeroPageAddr);
								AddTAC(tac, instruction.GetAddress());
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

bool TACTranslater::TranslateOperand(TAC& tac, const Instruction& instruction)
{
	const OpcodeEntry& entry = instruction.GetEntry();
	uint32_t addr;
	switch (entry.addrMode)
	{
	case AddrMode::Accumulator:
		tac.x = RegisterA;
		return false;
	case AddrMode::Immediate:
		tac.x = TACOperand(instruction.GetByte());
		return false;
	case AddrMode::Absolute:
		addr = instruction.GetOperandAddress();
		break;
	case AddrMode::AbsoluteX:
		tac.x = TACOperand(TACOperand::ADDRESS | instruction.GetOperandAddress());
		tac.y = RegisterX;
		return true;
	case AddrMode::AbsoluteY:
		tac.x = TACOperand(TACOperand::ADDRESS | instruction.GetOperandAddress());
		tac.y = RegisterY;
		return true;
	case AddrMode::Relative:
		tac.x = TACOperand(TACOperand::ADDRESS | instruction.GetConditionalJumpAddress());
		return false;
	case AddrMode::ZeroPage:
		addr = instruction.GetByte();
		break;
	case AddrMode::ZeroPageX:
		tac.x = TACOperand(TACOperand::ADDRESS | instruction.GetByte());
		tac.y = RegisterX;
		return true;
	case AddrMode::ZeroPageY:
		tac.x = TACOperand(TACOperand::ADDRESS | instruction.GetByte());
		tac.y = RegisterY;
		return true;
	case AddrMode::IndirectY:
	{
								// [Y + [zp]]
								// zp �������ַָ�룬�����ú�õ������ַ
								// ������ Y ��������õ�Ԫ��ֵ

								// 1. ��������һ����������ҳ��ַ������ַ��
								int zeroPageAddr = instruction.GetByte();
								TACOperand addrPointer(TACOperand::MEMORY | zeroPageAddr);
								// (1) ���һ����ʱ���������ڱ������ҳȡ���ĵ�ֵַ
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);
								// (2) ���ɽ����õ�����ַ��
								TAC* t = allocator.New<TAC>(TACOperator::DEREF, result, addrPointer);
								AddTAC(t, instruction.GetAddress());

								// 2. ���һ��ȡ����Ԫ�ص�����ַ��
								tac.x = result;
								tac.y = RegisterY;
								return true;
	}
	default:
	{
			   TCHAR buffer[64];
			   _stprintf_s(buffer, _T("%04X ��ȡָ��Ĳ�����δʵ�ֵ�Ѱַģʽ %s"), instruction.GetAddress(),
				   Nes::ToString(entry.addrMode));
			   throw Exception(buffer);
	}
	}
	// ����ȫ�ֱ���
	auto v = GetCDB().GetGlobalVariable(addr);
	if (v)
	{
		if (v->address != addr)
		{
			// ʹ��ƫ����������
			tac.x = TACOperand(TACOperand::ADDRESS | instruction.GetByte());
			tac.y = TACOperand(addr - v->address);
			return true;
		}
	}
	tac.x = TACOperand(TACOperand::MEMORY | addr);
	return false;
}

TAC* TACTranslater::TranslateCmp(const Instruction& instruction, const Instruction& next, TACOperand reg)
{
	TCHAR buffer[128];
	// ������һ����ʱ����ַ��ռλ
	auto tac = allocator.New<TAC>(TACOperator::NOP, TACOperand(), reg, GetOperand(instruction));
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
		tac->op = TACOperator::IFGEQ;
		break;
	case Opcode::Beq:
		tac->op = TACOperator::IFEQ;
	case Opcode::Bne:
		tac->op = TACOperator::IFNEQ;
		break;
	case Opcode::Bcc:
		tac->op = TACOperator::IFLESS;
		break;
		//case Opcode::Bvc:
		//case Opcode::Bvs:
	default:
		throw "δʵ��";
	}
	tac->z = TACOperand(TACOperand::ADDRESS | next.GetConditionalJumpAddress());
	return tac;
}

void TACTranslater::AddTAC(TAC* tac, Nes::Address address)
{
	// this->tacSub->AddTAC(tac);
	tac->address = address;
	this->tacBlock->AddTAC(tac);
}

void TACTranslater::SaveTACStart()
{
	this->tacStarts.push_back((int)this->tacBlock->GetCodesCount());
}

TAC* TACTranslater::GetBlockTAC(int index)
{
	auto& tacs = this->tacBlock->GetCodes();
	// һ��NESָ����ܶ�Ӧ��������ַ�룬��Ҫ���ص�������Ӧ�����һ������ַ��
	int end = (size_t)(index + 1) < this->tacStarts.size() ? this->tacStarts[index + 1] - 1 : (int)tacs.size() - 1;
	return tacs[end];
}
