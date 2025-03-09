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

	// 重构基本块的边
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

// index : 条件跳转指令的索引
// need : 需要的标志位
TAC* TACTranslater::TranslateConditionalJump(std::vector<Instruction>& instructions, size_t index, uint32_t need, TACOperator op)
{
	for (int i = (int)index - 1; i >= 0; --i)
	{
		auto& instruction = instructions[i];
		if ((instruction.GetEntry().kind & need) != 0)  // 写入了必要的标志位
		{
			// 查找这条指令对应的三地址码
			TAC* tac = GetBlockTAC(i);
			// 翻译成 if x ? y goto z 的形式
			return allocator.New<TAC>(op, GetOperand(instructions[index]), tac->z, TACOperand(0));
		}
	}
	TCHAR buffer[256];
	_stprintf_s(buffer, _T("地址 %04X 分析失败：跳转指令所在的基本块没有指令写入需要的标志位"), instructions[index].address);
	throw Exception(buffer);
}

TAC* TACTranslater::TranslateCall(Nes::Address callAddr, Nes::Address addr)
{
	int argCount = 0;
	TACOperand x(TACOperand::ADDRESS | callAddr);
	auto tac = allocator.New<TAC>(TACOperator::CALL, 0, x, 0);
	// 1. 如果要调用的函数有参数，那么先传递参数
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
	// 2. 如果调用的函数有返回值，那么使用返回值给AXY寄存器赋值
	if (sub && (sub->flag & SUBF_RETURN))
	{
		// (1) 首先用 axy 接收返回值
		tac->z = TACOperand(TACOperand::TEMP | GetAxy());
		// (2) 将 axy 的字段赋值给 AXY 寄存器
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
	// 没有返回值的话，直接返回
	if ((this->tacSub->flag & SUBF_RETURN) == 0)
		return allocator.New<TAC>(TACOperator::RETURN);

	// 给返回值字段赋值，需要用数组的方式来赋值
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
	// 首先构造指令队列，方便后面往前查找
	std::vector<Instruction> instructions(256);
	instructions.clear();
	db.GetInstructions(instructions, block->GetStartAddress(), block->GetEndAddress());

	this->tacBlock = allocator.New<TACBasicBlock>(block->GetStartAddress(), block->GetEndAddress());
	this->tacStarts.clear();

	// 遍历指令构造三地址码
	TAC* tac = nullptr;
	Instruction* last = nullptr;
	TAC temp;
	for (size_t index = 0; index < instructions.size(); ++index)
	{
		auto& i = instructions[index];
		const OpcodeEntry& entry = GetOpcodeEntry(i.GetOperatorByte());
		this->SaveTACStart();  // 记录这条指令对应的三地址码开始索引
		switch (entry.opcode)
		{
		case Nes::Opcode::None:
			continue;  // 忽略
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
								 // 可能是用JMP表示的尾调用
								 if (entry.addrMode == AddrMode::Absolute)
								 {
									 auto jumpAddr = i.GetOperandAddress();
									 if (jumpAddr < this->nesSub->GetStartAddress() ||
										 jumpAddr >= this->nesSub->GetEndAddress())
									 {
										 // 认为是尾调用
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
				   _stprintf_s(buffer, _T("NES指令转三地址码: 未实现的NES指令 %s"), ToString(entry.opcode));
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
								// 需要额外添加一条三地址码用于计算地址
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);  // 创建一个临时变量保存计算结果地址
								TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterX, instruction.GetOperandAddress());
								AddTAC(tac, instruction.GetAddress());
								// 然后返回临时变量作为地址
								result.SetKind(TACOperand::MEMORY);
								return result;
	}
	case AddrMode::AbsoluteY:
	{
								// 需要额外添加一条三地址码用于计算地址
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);  // 创建一个临时变量保存计算结果地址
								TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterX, instruction.GetOperandAddress());
								AddTAC(tac, instruction.GetAddress());
								// 然后返回临时变量
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
								// 需要额外添加一条三地址码用于计算地址
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);  // 创建一个临时变量保存计算结果地址
								TACOperand zeroPageAddr(TACOperand::MEMORY | instruction.GetByte());  // 从零页指定2字节单元取出地址
								TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterY, zeroPageAddr);
								AddTAC(tac, instruction.GetAddress());
								// 然后返回临时变量作为地址
								result.SetKind(TACOperand::MEMORY);
								return result;
	}
	default:
	{
			   TCHAR buffer[64];
			   _stprintf_s(buffer, _T("%04X 获取指令的操作数未实现的寻址模式 %s"), instruction.GetAddress(),
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
								// zp 是数组地址指针，解引用后得到数组地址
								// 接着用 Y 索引数组得到元素值

								// 1. 首先生成一条解引用零页地址的三地址码
								int zeroPageAddr = instruction.GetByte();
								TACOperand addrPointer(TACOperand::MEMORY | zeroPageAddr);
								// (1) 添加一个临时变量，用于保存从零页取出的地址值
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);
								// (2) 生成解引用的三地址码
								TAC* t = allocator.New<TAC>(TACOperator::DEREF, result, addrPointer);
								AddTAC(t, instruction.GetAddress());

								// 2. 添加一条取数组元素的三地址码
								tac.x = result;
								tac.y = RegisterY;
								return true;
	}
	default:
	{
			   TCHAR buffer[64];
			   _stprintf_s(buffer, _T("%04X 获取指令的操作数未实现的寻址模式 %s"), instruction.GetAddress(),
				   Nes::ToString(entry.addrMode));
			   throw Exception(buffer);
	}
	}
	// 查找全局变量
	auto v = GetCDB().GetGlobalVariable(addr);
	if (v)
	{
		if (v->address != addr)
		{
			// 使用偏移量来访问
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
	// 首先用一条临时三地址码占位
	auto tac = allocator.New<TAC>(TACOperator::NOP, TACOperand(), reg, GetOperand(instruction));
	// 要求下一条指令必须是条件跳转指令
	auto& entry = next.GetEntry();
	if ((entry.kind & OpKind::ConditionalJump) == 0)
	{
		_stprintf_s(buffer, _T("地址 %04X 分析失败：CMP指令的下一条指令不是跳转指令"), instruction.address);
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
		throw "未实现";
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
	// 一条NES指令可能对应多条三地址码，需要返回的是它对应的最后一条三地址码
	int end = (size_t)(index + 1) < this->tacStarts.size() ? this->tacStarts[index + 1] - 1 : (int)tacs.size() - 1;
	return tacs[end];
}
