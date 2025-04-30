#include "stdafx.h"
#include "TACTranslater1.h"
using namespace Nes;
#include "CDataBase.h"

TACTranslater1::TACTranslater1(NesDataBase& db_, Allocator& allocator_) :
db(db_),
allocator(allocator_),
tacSub(nullptr),
tacStarts(32)
{
}

TACTranslater1::~TACTranslater1()
{
}

//#include "TACFlagRegisterOptimizer.h"

TACFunction* TACTranslater1::Translate(NesSubroutine* subroutine)
{
	if (!subroutine)
		return nullptr;
	Reset();

	this->nesSub = subroutine;

	std::unordered_map<Nes::Address, TACBasicBlock*> blockMap;
	this->tacSub = allocator.New<TACFunction>(subroutine->GetStartAddress(), subroutine->GetEndAddress());
	this->tacSub->flag = subroutine->flag;

	auto& blocks = subroutine->GetBasicBlocks();
	for (auto block : blocks)
	{
		auto tacBlock = TranslateBasickBlock(block);
		//TACFlagRegisterOptimizer opt(allocator);
		//tacBlock->SetCodes(opt.Optimize(tacBlock->GetCodes()));
		this->tacSub->AddBasicBlock(tacBlock);
		blockMap[block->GetStartAddress()] = tacBlock;
	}

	// 重构基本块的边
	for (auto block : blocks)
	{
		auto tacBlock = blockMap[block->GetStartAddress()];

		for (auto prev : block->GetPreds())
			tacBlock->prevs.push_back(blockMap[prev]);
		for (auto succ : block->GetSuccs())
			tacBlock->nexts.push_back(blockMap[succ]);
		tacBlock->flag = block->GetFlag();
	}

	// 修正末尾基本块
	auto tail = blocks.back();
	if (tail->GetEndFlag() == BBF_END_NORMAL && !tail->GetSuccs().empty())
	{
		// 添加一条goto指令
		auto jumpAddr = tail->GetSuccs().front();
		auto tac = allocator.New<TAC>(TACOperator::GOTO, TACOperand(TACOperand::ADDRESS | jumpAddr));
		tac->address = tail->GetEndAddress() - 1;
		blockMap[tail->GetStartAddress()]->AddTAC(tac);
	}
	return this->tacSub;
}

void TACTranslater1::Reset()
{
	nesSub = nullptr;;
	tacSub = nullptr;;
	tacBlock = nullptr;
	tacStarts.clear();
	axy = -1;
}

TAC* TACTranslater1::TranslateCall(Nes::Address callAddr, Nes::Address addr)
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

TAC* TACTranslater1::TranslateReturn(Nes::Address address)
{
	// 没有返回值的话，直接返回
	if ((this->tacSub->flag & SUBF_RETURN) == 0)
		return allocator.New<TAC>(TACOperator::RETURN);

	// 给返回值字段赋值，需要用数组的方式来赋值
	auto returns = this->tacSub->GetReturnFlag();
	if (returns & (1 << Nes::NesRegisters::A))
	{
		auto tac = allocator.New<TAC>(TACOperator::ARRAY_SET, RegisterA,
			TACOperand(TACOperand::TEMP | GetAxy()), 0);
		AddTAC(tac, address);
	}
	if (returns & (1 << Nes::NesRegisters::X))
	{
		auto tac = allocator.New<TAC>(TACOperator::ARRAY_SET, RegisterX,
			TACOperand(TACOperand::TEMP | GetAxy()), 1);
		AddTAC(tac, address);
	}
	if (returns & (1 << Nes::NesRegisters::Y))
	{
		auto tac = allocator.New<TAC>(TACOperator::ARRAY_SET, RegisterY,
			TACOperand(TACOperand::TEMP | GetAxy()), 2);
		AddTAC(tac, address);
	}
	return allocator.New<TAC>(TACOperator::RETURN, 0, TACOperand(TACOperand::TEMP | GetAxy()));
}


TAC* TACTranslater1::TranslateJump(TACOperator op, const Instruction& instruction, TACOperand flag)
{
	TACOperand target = GetOperand(instruction);
	// 如果是跳转到函数外的地址，翻译为尾函数调用
	// if xxx goto A  =>
	// if! xxx goto L
	// CALL x
	// return
	// L: 
	if (!nesSub->Contains(target.GetValue()))
	{
		Nes::Address nextAddr = instruction.GetAddress() + instruction.GetLength();
		AddTAC(allocator.New<TAC>(GetNotOperator(op), TACOperand(TACOperand::ADDRESS | nextAddr), flag, 0), instruction.GetAddress());
		return GenerateTailCall(target, instruction.GetAddress());
	}
	return allocator.New<TAC>(op, target, flag);
}

int TACTranslater1::GetAxy()
{
	if (axy < 0)
		axy = this->tacSub->NewTemp(GetCDB().GetAXYType());
	return axy;
}

void TACTranslater1::TranslateFlag(const Instruction& intruction, TAC* tac)
{
	auto& entry = intruction.GetEntry();
	//if (entry.kind & Write_C)  // 需要在 tac 执行之前计算
	//{
	//	// 结果操作数需要提升到16位
	//	TACOperand r = NewTemp(TypeManager::Short);
	//	AddTAC(allocator.New<TAC>(tac->op, r, tac->x, tac->y), intruction.GetAddress());  // r = x op y
	//	// 接下来判断结果是否大于255
	//	AddTAC(allocator.New<TAC>(TACOperator::BOOL_GREAT, RegisterC, r, 0xFF), intruction.GetAddress());  // r = x > 255
	//}
	//if (entry.kind & Write_V)  // 需要在 tac 执行之前计算
	//{
	//	// 溢出判断：两个正数相加得负数或两个负数相加得正数
	//	// 判断算法：两个操作数的符号位相同（异或后符号位是0），
	//	//    但结果的符号位与它们不同（异或后符号位是1）
	//	//    再异或（符号位为1）
	//	//  V = ((x ^ y) & 0x80) == 0  && ((x ^ z) & 0x80) == 1

	//	AddTAC(allocator.New<TAC>(TACOperator::BOOL_BAND, RegisterV, RegisterA, 0x80), intruction.GetAddress());
	//}

	//AddTAC(tac, intruction.GetAddress());

	if (entry.kind & Write_N)
	{
		// AddTAC(allocator.New<TAC>(TACOperator::BOOL_BAND, RegisterN, tac->z, 0x80), intruction.GetAddress());
		AddTAC(allocator.New<TAC>(TACOperator::BOOL_LESS, RegisterN, tac->z, 0), intruction.GetAddress());
	}
	if (entry.kind & Write_Z)
	{
		AddTAC(allocator.New<TAC>(TACOperator::BOOL_EQ, RegisterZ, tac->z, 0), intruction.GetAddress());
	}
}

TACOperand TACTranslater1::GetSharedTemp()
{
	if (this->temp.IsInterger())
	{
		this->temp = NewTemp(TypeManager::Value);
	}
	return this->temp;
}

TAC* TACTranslater1::GenerateTailCall(TACOperand target, Nes::Address address)
{
	AddTAC(TranslateCall(target.GetValue(), address), address);  // target 必定是地址常数
	return TranslateReturn(address);
}

TACBasicBlock* TACTranslater1::TranslateBasickBlock(NesBasicBlock* block)
{
	// 首先构造指令队列，方便后面往前查找
	std::vector<Instruction> instructions(256);
	instructions.clear();
	db.GetInstructions(instructions, block->GetStartAddress(), block->GetEndAddress());

	this->tacBlock = allocator.New<TACBasicBlock>(block->GetStartAddress(), block->GetEndAddress());
	this->tacStarts.clear();
	this->temp = 0;

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
			// tac = allocator.New<TAC>(TACOperator::NOP);
			break;
		case Nes::Opcode::Asl:
			// C = 最高位
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_BAND, RegisterC, GetOperand(i), 0x80), i.GetAddress());
			tac = allocator.New<TAC>(TACOperator::SHL, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Lsr:
			// C = 最低位
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_BAND, RegisterC, GetOperand(i), 1), i.GetAddress());
			tac = allocator.New<TAC>(TACOperator::SHR, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Rol:
			// C = 最高位
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_BAND, RegisterC, GetOperand(i), 0x80), i.GetAddress());
			tac = allocator.New<TAC>(TACOperator::ROL, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Ror:
			// C = 最低位
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_BAND, RegisterC, GetOperand(i), 1), i.GetAddress());
			tac = allocator.New<TAC>(TACOperator::ROR, GetOperand(i), GetOperand(i), TACOperand(1));
			break;
		case Nes::Opcode::Bpl:
			tac = TranslateJump(TACOperator::IFFALSE, i, RegisterN);
			break;
		case Nes::Opcode::Bmi:
			tac = TranslateJump(TACOperator::IFTRUE, i, RegisterN);
			break;
		case Nes::Opcode::Bne:
			tac = TranslateJump(TACOperator::IFFALSE, i, RegisterZ);
			break;
		case Nes::Opcode::Beq:
			tac = TranslateJump(TACOperator::IFTRUE, i, RegisterZ);
			break;
		case Nes::Opcode::Bcc:
			tac = TranslateJump(TACOperator::IFFALSE, i, RegisterC);
			break;
		case Nes::Opcode::Bcs:
			tac = TranslateJump(TACOperator::IFTRUE, i, RegisterC);
			break;
		case Nes::Opcode::Bvc:
			tac = TranslateJump(TACOperator::IFFALSE, i, RegisterV);
			break;
		case Nes::Opcode::Bvs:
			tac = TranslateJump(TACOperator::IFTRUE, i, RegisterV);
			break;
		case Nes::Opcode::Cli:
			tac = allocator.New<TAC>(TACOperator::CLI);
			break;
		case Nes::Opcode::Sei:
			tac = allocator.New<TAC>(TACOperator::SEI);
			break;
		case Nes::Opcode::Clc:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterC, 0);
			break;
		case Nes::Opcode::Sec:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterC, 1);
			break;
		case Nes::Opcode::Clv:
			tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterV, 0);
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
			// N = M.7, V = M.6
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_BIT, RegisterN, GetOperand(i), 7), i.GetAddress());
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_BIT, RegisterV, GetOperand(i), 6), i.GetAddress());
			tac = allocator.New<TAC>(TACOperator::BAND, NewTemp(TypeManager::Value), RegisterA, GetOperand(i));
			AddTAC(tac, i.GetAddress());
			// z = t == 0
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_EQ, RegisterZ, tac->z, 0), i.GetAddress());
			continue;
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
		//case Nes::Opcode::Php:
		//	tac = allocator.New<TAC>(TACOperator::PUSH);
		//	tac->x = RegisterP;
		//	break;
		//case Nes::Opcode::Plp:
		//	tac = allocator.New<TAC>(TACOperator::POP, RegisterP);
		//	break;
		case Nes::Opcode::Jmp:
		{
								 // 可能是用JMP表示的尾调用
								 if (entry.addrMode == AddrMode::Absolute)
								 {
									 auto jumpAddr = i.GetOperandAddress();
									 if (!this->nesSub->Contains(jumpAddr))
									 {
										 // 认为是尾调用
										 tac = GenerateTailCall(i.GetOperandAddress(), i.GetAddress());
										 break;
									 }
								 }
								 else
								 {
									 // 间接寻址相当于尾函数调用
									 auto addr = i.GetOperandAddress();  // 函数指针地址
									 auto pFunc = GetCDB().GetGlobalVariable(addr);  // 获取函数指针全局变量
									 auto pfType = pFunc->type;
									 assert(pfType->GetKind() == TypeKind::Pointer);
									 assert(pfType->pa.type->GetKind() == TypeKind::Function);
									 // 函数返回值和参数目前还没实现，先当作没有处理
									 // 1. 先生成一条解引用指令
									 // tac = allocator.New<TAC>(TACOperator::DEREF, NewTemp(pfType->pa.type), GetOperand(i));
									 // 2. 生成尾函数调用指令
									 tac = GenerateTailCall(GetOperand(i), i.GetAddress());
									 break;
								 }
								 tac = allocator.New<TAC>(TACOperator::GOTO, GetOperand(i));
								 tac->z.SetKind(TACOperand::ADDRESS);
								 break;
		}
		case Nes::Opcode::Rts:
		case Nes::Opcode::Rti:
			tac = TranslateReturn(i.GetAddress());
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
			// C = A >= M
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_GEQ, RegisterC, RegisterA, GetOperand(i)), i.GetAddress());
			tac = allocator.New<TAC>(TACOperator::SUB, GetSharedTemp(), RegisterA, GetOperand(i));
			break;
		case Nes::Opcode::Cpx:
			// C = X >= M
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_GEQ, RegisterC, RegisterX, GetOperand(i)), i.GetAddress());
			tac = allocator.New<TAC>(TACOperator::SUB, GetSharedTemp(), RegisterX, GetOperand(i));
			break;
		case Nes::Opcode::Cpy:
			// C = Y >= M
			AddTAC(allocator.New<TAC>(TACOperator::BOOL_GEQ, RegisterC, RegisterY, GetOperand(i)), i.GetAddress());
			tac = allocator.New<TAC>(TACOperator::SUB, GetSharedTemp(), RegisterY, GetOperand(i));
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
		case Nes::Opcode::Adc:
		{
								 TACOperand r = NewTemp(TypeManager::Short);
								 // r = A + M
								 AddTAC(allocator.New<TAC>(TACOperator::ADD, r, RegisterA, GetOperand(i)), i.GetAddress());
								 // r += C
								 AddTAC(allocator.New<TAC>(TACOperator::ADD, r, r, RegisterC), i.GetAddress());
								 // V = 
								 AddTAC(allocator.New<TAC>(TACOperator::BOOL_FLAGV, A, GetOperand(i)), i.GetAddress());
								 // C = (r & 0x100) != 0
								 AddTAC(allocator.New<TAC>(TACOperator::BOOL_BAND, r, 0x100), i.GetAddress());
								 // A = r
								 tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterA, r), i.GetAddress();
								 break;
		}
		case Nes::Opcode::Sbc:
		{
								 TACOperand r = NewTemp(TypeManager::Short);
								 // r = A - M
								 AddTAC(allocator.New<TAC>(TACOperator::SUB, r, RegisterA, GetOperand(i)), i.GetAddress());
								 // t = ~C
								 TACOperand t = NewTemp(TypeManager::Value);
								 AddTAC(allocator.New<TAC>(TACOperator::BNOT, t, RegisterC), i.GetAddress());
								 // r -= t
								 AddTAC(allocator.New<TAC>(TACOperator::SUB, r, r, t), i.GetAddress());
								 // V = 
								 AddTAC(allocator.New<TAC>(TACOperator::BOOL_FLAGV, A, GetOperand(i)), i.GetAddress());
								 // C = r <= 255
								 AddTAC(allocator.New<TAC>(TACOperator::BOOL_LEQ, r, 0x100), i.GetAddress());
								 // A = r
								 tac = allocator.New<TAC>(TACOperator::ASSIGN, RegisterA, r), i.GetAddress();
								 break;
		}
		case Nes::Opcode::Sax:
			tac = allocator.New<TAC>(TACOperator::BAND, GetOperand(i), RegisterA, RegisterX);
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
			TranslateFlag(i, tac);
			tac = nullptr;
		}
		last = &i;
	}
	return tacBlock;
}

TACOperand TACTranslater1::GetOperand(const Instruction& instruction)
{
	const OpcodeEntry& entry = instruction.GetEntry();
	switch (entry.addrMode)
	{
	case AddrMode::Accumulator:
		return RegisterA;
	case AddrMode::Immediate:
		return TACOperand(instruction.GetByte());
	case AddrMode::Absolute:
		return TACOperand(TACOperand::GLOBAL | instruction.GetOperandAddress());
	case AddrMode::AbsoluteX:
	{
								// 需要额外添加一条三地址码用于计算地址
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);  // 创建一个临时变量保存计算结果地址
								TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterX, instruction.GetOperandAddress());
								AddTAC(tac, instruction.GetAddress());
								return result;
	}
	case AddrMode::AbsoluteY:
	{
								// 需要额外添加一条三地址码用于计算地址
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);  // 创建一个临时变量保存计算结果地址
								TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterX, instruction.GetOperandAddress());
								AddTAC(tac, instruction.GetAddress());
								return result;
	}
	case AddrMode::Relative:
		return TACOperand(TACOperand::ADDRESS | instruction.GetConditionalJumpAddress());
	case AddrMode::ZeroPage:
		return TACOperand(TACOperand::GLOBAL | instruction.GetByte());
	//case AddrMode::ZeroPageX:
	//{
	//	// 需要额外添加一条三地址码用于计算地址
	//	int temp = this->tacSub->NewTemp(TypeManager::pValue);
	//	TACOperand result(TACOperand::TEMP | temp);  // 创建一个临时变量保存计算结果地址
	//	TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterX, instruction.GetOperandAddress());
	//	AddTAC(tac, instruction.GetAddress());
	//	return result;
	//}
	case AddrMode::Indirect:
		return TACOperand(TACOperand::GLOBAL | instruction.GetOperandAddress());
	case AddrMode::IndirectY:
	{
								// [Y + [zp]]
								// 需要额外添加一条三地址码用于计算地址
								int temp = this->tacSub->NewTemp(TypeManager::pValue);
								TACOperand result(TACOperand::TEMP | temp);  // 创建一个临时变量保存计算结果地址
								TACOperand zeroPageAddr(TACOperand::GLOBAL | instruction.GetByte());  // 从零页指定2字节单元取出地址
								TAC* tac = allocator.New<TAC>(TACOperator::ADD, result, RegisterY, zeroPageAddr);
								AddTAC(tac, instruction.GetAddress());
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

TACOperand TACTranslater1::NewTemp(Type* type)
{
	int temp = this->tacSub->NewTemp(type);
	return TACOperand(TACOperand::TEMP | temp);
}

bool TACTranslater1::TranslateOperand(TAC& tac, const Instruction& instruction)
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
		tac.x = TACOperand(TACOperand::GLOBAL | instruction.GetOperandAddress());
		tac.y = RegisterX;
		return true;
	case AddrMode::AbsoluteY:
		tac.x = TACOperand(TACOperand::GLOBAL | instruction.GetOperandAddress());
		tac.y = RegisterY;
		return true;
	case AddrMode::Relative:
		tac.x = TACOperand(TACOperand::ADDRESS | instruction.GetConditionalJumpAddress());
		return false;
	case AddrMode::ZeroPage:
		addr = instruction.GetByte();
		break;
	case AddrMode::ZeroPageX:
		tac.x = TACOperand(TACOperand::GLOBAL | instruction.GetByte());
		tac.y = RegisterX;
		return true;
	case AddrMode::ZeroPageY:
		tac.x = TACOperand(TACOperand::GLOBAL | instruction.GetByte());
		tac.y = RegisterY;
		return true;
	case AddrMode::IndirectX:
	{
								// [[zp + X]] 对应的C代码为
								// char g_zp[];
								// char X;
								// char A = *(char*)g_zp[X];
								// 1. 取数组元素
								auto code = allocator.New<TAC>(TACOperator::ARRAY_GET, NewTemp(TypeManager::Value));
								code->x = TACOperand(TACOperand::GLOBAL | instruction.GetByte());
								code->y = RegisterY;
								AddTAC(code, instruction.GetAddress());
								// 2. 类型转换 char -> char*
								code = allocator.New<TAC>(TACOperator::CAST, NewTemp(TypeManager::pValue), code->z);
								AddTAC(code, instruction.GetAddress());
								// 3. 解引用
								code = allocator.New<TAC>(TACOperator::DEREF, NewTemp(TypeManager::Value), code->z);
								AddTAC(code, instruction.GetAddress());
								tac.x = code->z;
								return false;
	}
	case AddrMode::IndirectY:
	{
								// [Y + [zp]] 对应的C代码为
								// char* g_zp;  // zp = &g_zp
								// 翻译为 g_zp[Y]
								tac.x = TACOperand(TACOperand::GLOBAL | instruction.GetByte());
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
	if (v && GetTypeBytes(v->type) != 1)
	{
		//if (v->address != addr)
		//{
		// 对于NES，这种情况必是2字节的指针，使用偏移量来访问
		// p = &a
		TACOperand global = TACOperand(TACOperand::GLOBAL | v->address);
		Type* type = GetTypeManager().NewPointer(v->type);
		TAC* code = allocator.New<TAC>(TACOperator::ADDR, NewTemp(type), global);
		AddTAC(code, instruction.GetAddress());
		// t = (char*)p, t 的类型必然是 char，因为 6502 一次只能操作8位
		code = allocator.New<TAC>(TACOperator::CAST, NewTemp(TypeManager::pValue), code->z);
		AddTAC(code, instruction.GetAddress());
		// 返回 t[n]，n 是偏移量，对于 6502，总是0, 或 1
		tac.x = code->z;
		tac.y = TACOperand(addr - v->address);
		return true;
		//}
	}
	tac.x = TACOperand(TACOperand::GLOBAL | addr);
	return false;
}

void TACTranslater1::AddTAC(TAC* tac, Nes::Address address)
{
	// this->tacSub->AddTAC(tac);
	tac->address = address;
	this->tacBlock->AddTAC(tac);
	// COUT << tac << std::endl;
}

void TACTranslater1::SaveTACStart()
{
	this->tacStarts.push_back((int)this->tacBlock->GetCodesCount());
}

TAC* TACTranslater1::GetBlockTAC(int index)
{
	auto& tacs = this->tacBlock->GetCodes();
	// 一条NES指令可能对应多条三地址码，需要返回的是它对应的最后一条三地址码
	int end = (size_t)(index + 1) < this->tacStarts.size() ? this->tacStarts[index + 1] - 1 : (int)tacs.size() - 1;
	return tacs[end];
}
