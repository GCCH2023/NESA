#pragma once

namespace Nes
{
	// 地址类型
	using Address = uint16_t;


	// 游戏的排列方式
	enum Alignment
	{
		Horizontal = 0,  // 水平
		Vertical = 1,  // 垂直
	};

	// 字节各位的标志
	enum Flags
	{
		C = 1 << 0,    // 进位 (Carry flag)
		Z = 1 << 1,    // 零 (Zero flag)
		I = 1 << 2,    // 禁止中断 (Irq disabled flag)
		D = 1 << 3,    // 十进制模式 (Decimal mode flag)
		B = 1 << 4,    // 软件中断 (BRK flag)
		U = 1 << 5,    // 保留 (Unuse), 一直为1
		V = 1 << 6,    // 溢出 (Overflow  flag)
		N = 1 << 7,    // 符号 (Negative flag)
	};

	// 状态标志位
	union Byte
	{
		uint8_t value;
		struct
		{
			uint8_t C : 1;
			uint8_t Z : 1;
			uint8_t I : 1;
			uint8_t D : 1;
			uint8_t B : 1;
			uint8_t U : 1;
			uint8_t V : 1;
			uint8_t N : 1;
		};
	};

	// 中断向量
	enum InterruptVector
	{
		NMI = 0xFFFA,
		RESET = 0xFFFC,
		IRQ = 0xFFFE
	};

	// 寻址模式
	enum AddrMode
	{
		// 隐式寻址 [隐式] Implied Addressing [Implied]
		// 在隐式寻址模式里， 包含操作数的地址被隐含在指令的操作码里。
		Implied,
		// 累加器寻址 [累加器] Accumulator Addressing [Accumulator]
		// 这种寻址模式表现为只有一个操作数，另一个操作数隐含为累加器。
		Accumulator,
		// 直接寻址 [直接] Immediate Addressing [Immediate]
		// 在直接寻址模式里，操作数被包含在指令的第二个字节里，不需要其他内存寻址。
		Immediate,
		// 0页面寻址 [0页面] Zero Page Addressing [Zero Page]
		// 0页面寻址允许只取指令的第二字节并假设高位地址为0，以得到较短的代码和执行时间。
		// 小心地使用0页面寻址能有效地增加代码的效率。
		ZeroPage,
		// 0页面变址寻址 [0页面,X 或 0页面,Y] Zero Page Indexed Addressing [Zero Page,X or Zero Page,Y]
		// 0页面变址寻址联合X或Y变址寄存器使用，以“0页面, X”和“0页面, Y”的形式出现。
		// 有效地址由指令的第二个字节加上变址寄存器的内容得到。
		// 因为这是一个“0页面”寻址，所以第二字节的内容定位于0页面。
		// 另外，因为“0页面”寻址模式的特性，所以不会有进位加到高位内存，也不会发生跨页边界的问题。
		ZeroPageX,
		ZeroPageY,
		// 相对寻址 [相对] Relative Addressing [Relative]
		// 相对寻址只用在转移（branch）指令指令中；它指定了条件转移的目标。
		// 指令的第二个字节变成一个操作数，作为偏移加到指向下一条指令的指令指针上。
		// 偏移范围从-128到127字节，相对于下一条指令。
		Relative,
		// 绝对寻址 [绝对] Absolute Addressing [Absolute]
		// 在绝对寻址模式里，指令的第二个字节指定了有效地址的低八位，第三个字节指定高八位。
		// 因此，这种寻址模式允许访问全部的64K可寻址内存。
		Absolute,
		// 绝对变址寻址 [绝对,X 或 绝对,Y] Absolute Indexed Addressing [Absolute,X or Absolute,Y]
		// 绝对变址寻址联合X或Y变址寄存器使用，以“绝对, X”和“绝对, Y”的形式出现。
		// 有效地址由X或Y的内容加上指令的第二或第三字节包含的地址得到。
		// 这种模式允许变址寄存器包含变址或计数值而指令包含基址。
		// 这种寻址模式允许定位任何位置并且可以用变址修改多个地方，用以减少代码和指令时间。
		AbsoluteX,
		AbsoluteY,
		Indirect,
		// 0页面变址间接寻址 [(0页面,X)] Zero Page Indexed Indirect Addressing [(Zero Page,X)
		// 
		// 0页面变址间接寻址（通常参考为 间接 X）指令的第二个字节加到X变址寄存器的内容上；进位被舍弃。
		// 加法运算的结果指向0页面的一个内存单元，而这个内存单元是有效地址的低8位。
		// 下一个0页面单元是有效地址的高8位。指向有效地址的高8位和低8位都必须在0页面内。
		IndirectX,
		// [Y + [M]]
		IndirectY,
	};

	// 根据寻址模式获取指令的字节数
	int GetInstructionBytesCount(AddrMode addrMode);
	const TCHAR* ToString(AddrMode addrMode);

	// NES 操作码枚举
	enum class Opcode
	{
		None,  // 未定义
		Brk, Ora, Slo, Nop, Asl, Lsr, Php,
		Bpl, Bmi,
		Clc, Jsr, And, Bit, Rol, Rla, Plp,
		Sec, Rti, Eor, Sre, Pha, Jmp,
		Bvc, Cli, Rts, Adc, Ror, Rra, Pla,
		Bvs, Sei, Sta, Sax, Sty, Stx, Dey, Txa,
		Bcc, Tya, Txs, Lda, Ldx, Ldy, Clv, Tsx,
		Cpy, Cmp, Dcp, Dec, Iny, Dex, Bne,
		Cld, Cpx, Sbc, Isc, Inc, Inx, Beq, Sed,
		Lax, Tay, Tax, Bcs
	};

	// 获取操作码的字符串表示
	const TCHAR* ToString(Opcode opcode);

	// 操作码和寻址模式枚举
	enum OpAddr
	{
		OPA_BRK_IMP = 0x00,
		OPA_ORA_INX = 0x01,

		OPA_SLO_INX = 0x03,
		OPA_NOP_04_ZP = 0x04,
		OPA_ORA_ZP = 0x05,
		OPA_ASL_ZP = 0x06,
		OPA_SLO_ZP = 0x07,
		OPA_PHP_IMP = 0x08,
		OPA_ORA_IMM = 0x09,
		OPA_ASL_ACC = 0x0A,

		OPA_NOP_0C_ABS = 0x0C,
		OPA_ORA_ABS = 0x0D,
		OPA_ASL_ABS = 0x0E,
		OPA_SLO_ABS = 0x0F,
		OPA_BPL_REL = 0x10,
		OPA_ORA_INY = 0x11,

		OPA_SLO_INY = 0x13,
		OPA_NOP_14_ZPX = 0x14,
		OPA_ORA_ZPX = 0x15,
		OPA_ASL_ZPX = 0x16,
		OPA_SLO_ZPX = 0x17,
		OPA_CLC_IMP = 0x18,
		OPA_ORA_ABY = 0x19,
		OPA_NOP_1A_IMP = 0x1A,
		OPA_SLO_ABY = 0x1B,
		OPA_NOP_1C_ABX = 0x1C,
		OPA_ORA_ABX = 0x1D,
		OPA_ASL_ABX = 0x1E,
		OPA_SLO_ABX = 0x1F,
		OPA_JSR_ABS = 0x20,
		OPA_AND_INX = 0x21,

		OPA_RLA_INX = 0x23,
		OPA_BIT_ZP = 0x24,
		OPA_AND_ZP = 0x25,
		OPA_ROL_ZP = 0x26,
		OPA_RLA_ZP = 0x27,
		OPA_PLP_IMP = 0x28,
		OPA_AND_IMM = 0x29,
		OPA_ROL_ACC = 0x2A,

		OPA_BIT_ABS = 0x2C,
		OPA_AND_ABS = 0x2D,
		OPA_ROL_ABS = 0x2E,
		OPA_RLA_ABS = 0x2F,
		OPA_BMI_REL = 0x30,
		OPA_AND_INY = 0x31,

		OPA_RLA_INY = 0x33,
		OPA_NOP_34_ZPX = 0x34,
		OPA_AND_ZPX = 0x35,
		OPA_ROL_ZPX = 0x36,
		OPA_RLA_ZPX = 0x37,
		OPA_SEC_IMP = 0x38,
		OPA_AND_ABY = 0x39,
		OPA_NOP_3A_IMP = 0x3A,
		OPA_RLA_ABY = 0x3B,
		OPA_NOP_3C_ABX = 0x3C,
		OPA_AND_ABX = 0x3D,
		OPA_ROL_ABX = 0x3E,
		OPA_RLA_ABX = 0x3F,
		OPA_RTI_IMP = 0x40,
		OPA_EOR_INX = 0x41,

		OPA_SRE_INX = 0x43,
		OPA_NOP_44_ZP = 0x44,
		OPA_EOR_ZP = 0x45,
		OPA_LSR_ZP = 0x46,
		OPA_SRE_ZP = 0x47,
		OPA_PHA_IMP = 0x48,
		OPA_EOR_IMM = 0x49,
		OPA_LSR_ACC = 0x4A,

		OPA_JMP_ABS = 0x4C,
		OPA_EOR_ABS = 0x4D,
		OPA_LSR_ABS = 0x4E,
		OPA_SRE_ABS = 0x4F,
		OPA_BVC_REL = 0x50,
		OPA_EOR_INY = 0x51,

		OPA_SRE_INY = 0x53,
		OPA_NOP_54_ZPX = 0x54,
		OPA_EOR_ZPX = 0x55,
		OPA_LSR_ZPX = 0x56,
		OPA_SRE_ZPX = 0x57,
		OPA_CLI_IMP = 0x58,
		OPA_EOR_ABY = 0x59,
		OPA_NOP_5A_IMP = 0x5A,
		OPA_SRE_ABY = 0x5B,
		OPA_NOP_5C_ABX = 0x5C,
		OPA_EOR_ABX = 0x5D,
		OPA_LSR_ABX = 0x5E,
		OPA_SRE_ABX = 0x5F,
		OPA_RTS_IMP = 0x60,
		OPA_ADC_INX = 0x61,

		OPA_RRA_INX = 0x63,
		OPA_NOP_64_ZP = 0x64,
		OPA_ADC_ZP = 0x65,
		OPA_ROR_ZP = 0x66,
		OPA_RRA_ZP = 0x67,
		OPA_PLA_IMP = 0x68,
		OPA_ADC_IMM = 0x69,
		OPA_ROR_ACC = 0x6A,

		OPA_JMP_IND = 0x6C,
		OPA_ADC_ABS = 0x6D,
		OPA_ROR_ABS = 0x6E,
		OPA_RRA_ABS = 0x6F,
		OPA_BVS_REL = 0x70,
		OPA_ADC_INY = 0x71,

		OPA_RRA_INY = 0x73,
		OPA_NOP_74_ZPX = 0x74,
		OPA_ADC_ZPX = 0x75,
		OPA_ROR_ZPX = 0x76,
		OPA_RRA_ZPX = 0x77,
		OPA_SEI_IMP = 0x78,
		OPA_ADC_ABY = 0x79,
		OPA_NOP_7A_IMP = 0x7A,
		OPA_RRA_ABY = 0x7B,
		OPA_NOP_7C_ABX = 0x7C,
		OPA_ADC_ABX = 0x7D,
		OPA_ROR_ABX = 0x7E,
		OPA_RRA_ABX = 0x7F,
		OPA_NOP_80_IMM = 0x80,
		OPA_STA_INX = 0x81,
		OPA_NOP_82_IMM = 0x82,
		OPA_SAX_INX = 0x83,
		OPA_STY_ZP = 0x84,
		OPA_STA_ZP = 0x85,
		OPA_STX_ZP = 0x86,
		OPA_SAX_ZP = 0x87,
		OPA_DEY_IMP = 0x88,

		OPA_TXA_IMP = 0x8A,

		OPA_STY_ABS = 0x8C,
		OPA_STA_ABS = 0x8D,
		OPA_STX_ABS = 0x8E,
		OPA_SAX_ABS = 0x8F,
		OPA_BCC_REL = 0x90,
		OPA_STA_INY = 0x91,


		OPA_STY_ZPX = 0x94,
		OPA_STA_ZPX = 0x95,
		OPA_STX_ZPY = 0x96,
		OPA_SAX_ZPY = 0x97,
		OPA_TYA_IMP = 0x98,
		OPA_STA_ABY = 0x99,
		OPA_TXS_IMP = 0x9A,


		OPA_STA_ABX = 0x9D,


		OPA_LDY_IMM = 0xA0,
		OPA_LDA_INX = 0xA1,
		OPA_LDX_IMM = 0xA2,
		OPA_LAX_INX = 0xA3,
		OPA_LDY_ZP = 0xA4,
		OPA_LDA_ZP = 0xA5,
		OPA_LDX_ZP = 0xA6,
		OPA_LAX_ZP = 0xA7,
		OPA_TAY_IMP = 0xA8,
		OPA_LDA_IMM = 0xA9,
		OPA_TAX_IMP = 0xAA,

		OPA_LDY_ABS = 0xAC,
		OPA_LDA_ABS = 0xAD,
		OPA_LDX_ABS = 0xAE,
		OPA_LAX_ABS = 0xAF,
		OPA_BCS_REL = 0xB0,
		OPA_LDA_INY = 0xB1,

		OPA_LAX_INY = 0xB3,
		OPA_LDY_ZPX = 0xB4,
		OPA_LDA_ZPX = 0xB5,
		OPA_LDX_ZPY = 0xB6,
		OPA_LAX_ZPY = 0xB7,
		OPA_CLV_IMP = 0xB8,
		OPA_LDA_ABY = 0xB9,
		OPA_TSX_IMP = 0xBA,

		OPA_LDY_ABX = 0xBC,
		OPA_LDA_ABX = 0xBD,
		OPA_LDX_ABY = 0xBE,
		OPA_LAX_ABY = 0xBF,
		OPA_CPY_IMM = 0xC0,
		OPA_CMP_INX = 0xC1,

		OPA_DCP_INX = 0xC3,
		OPA_CPY_ZP = 0xC4,
		OPA_CMP_ZP = 0xC5,
		OPA_DEC_ZP = 0xC6,
		OPA_DCP_ZP = 0xC7,
		OPA_INY_IMP = 0xC8,
		OPA_CMP_IMM = 0xC9,
		OPA_DEX_IMP = 0xCA,

		OPA_CPY_ABS = 0xCC,
		OPA_CMP_ABS = 0xCD,
		OPA_DEC_ABS = 0xCE,
		OPA_DCP_ABS = 0xCF,
		OPA_BNE_REL = 0xD0,
		OPA_CMP_INY = 0xD1,

		OPA_DCP_INY = 0xD3,
		OPA_NOP_D4_ZPX = 0xD4,
		OPA_CMP_ZPX = 0xD5,
		OPA_DEC_ZPX = 0xD6,
		OPA_DCP_ZPX = 0xD7,
		OPA_CLD_IMP = 0xD8,
		OPA_CMP_ABY = 0xD9,
		OPA_NOP_DA_IMP = 0xDA,
		OPA_DCP_ABY = 0xDB,
		OPA_NOP_DC_ABX = 0xDC,
		OPA_CMP_ABX = 0xDD,
		OPA_DEC_ABX = 0xDE,
		OPA_DCP_ABX = 0xDF,
		OPA_CPX_IMM = 0xE0,
		OPA_SBC_INX = 0xE1,

		OPA_ISC_INX = 0xE3,
		OPA_CPX_ZP = 0xE4,
		OPA_SBC_ZP = 0xE5,
		OPA_INC_ZP = 0xE6,
		OPA_ISC_ZP = 0xE7,
		OPA_INX_IMP = 0xE8,
		OPA_SBC_E9_IMM = 0xE9,
		OPA_NOP_EA_IMP = 0xEA,
		OPA_SBC_IMM = 0xEB,
		OPA_CPX_ABS = 0xEC,
		OPA_SBC_ABS = 0xED,
		OPA_INC_ABS = 0xEE,
		OPA_ISC_ABS = 0xEF,
		OPA_BEQ_REL = 0xF0,
		OPA_SBC_INY = 0xF1,

		OPA_ISC_INY = 0xF3,
		OPA_NOP_F4_ZPX = 0xF4,
		OPA_SBC_ZPX = 0xF5,
		OPA_INC_ZPX = 0xF6,
		OPA_ISC_ZPX = 0xF7,
		OPA_SED_IMP = 0xF8,
		OPA_SBC_ABY = 0xF9,
		OPA_NOP_FA_IMP = 0xFA,
		OPA_ISC_ABY = 0xFB,
		OPA_NOP_FC_ABX = 0xFC,
		OPA_SBC_ABX = 0xFD,
		OPA_INC_ABX = 0xFE,
		OPA_ISC_ABX = 0xFF,
	};

	// 指令的分类
	// 各个部分可以
	enum OpKind
	{
		None = 0,       // 其他类
		// 第一部分, 按功能划分
		Logical = 0x1,        // 逻辑
		Arithmetic = 0x2,     // 算术
		LogicalArithmetic = 0x3,    // 逻辑或算术
		Move = 0x4,           // 传输
		ConditionalJump = 0x8,    // 条件跳转
		UnconditionalJump = 0x10,     // 无条件跳转
		Call = 0x20,            // 函数调用
		Return = 0x40,        // 函数返回
		Jump = ConditionalJump | UnconditionalJump | Call | Return,     // 跳转指令
		// 第二部分, 是否会影响指定寄存器
		ReadA = 0x100,  // 读取寄存器A
		WriteA = 0x200,  // 写入寄存器A
		RWa = 0x300,    // 读取或写入寄存器A
		ReadX = 0x400,  // 读取寄存器X
		WriteX = 0x800,  // 写入寄存器X
		RWx = 0xC00,    // 读取或写入寄存器X
		ReadY = 0x1000,  // 读取寄存器Y
		WriteY = 0x2000,  // 写入寄存器Y
		RWy = 0x3000,    // 读取或写入寄存器Y
		ReadPC = 0x4000,  // 读取寄存器PC
		WritePC = 0x8000,  // 写入寄存器PC
		RWpc = 0xC000,    // 读取或写入寄存器PC
		ReadSP = 0x10000,  // 读取寄存器SP
		WriteSP = 0x20000,  // 写入寄存器SP
		RWsp = 0x30000,    // 读取或写入寄存器SP
		ReadM = 0x10000,  // 读取内存
		WriteM = 0x20000,  // 写入内存
		RWm = 0x30000,    // 读取或写入内存
		// 状态寄存器
		Write_C = 0x100000,     // 影响进位标志
		Write_Z = 0x200000,     // 影响零标志
		Write_I = 0x400000,     // 影响中断屏蔽标志
		Write_D3 = 0x800000,
		Write_B = 0x1000000,
		Write_D5 = 0x1000000,
		Write_V = 0x4000000,     // 影响溢出标志
		Write_N = 0x8000000,     // 影响负数标志
		ReadP = 0x10000000,     // 读取标志寄存器
		WriteP = 0xF00000,      // 写入标志寄存器
		RWp = 0x1F00000,     // 影响标志寄存器

		NZ = Write_N | Write_Z,
		BI = Write_B | Write_I,
		NZC = Write_N | Write_Z | Write_C,
		NVZC = NZC | Write_V,
		NVZ = Write_N | Write_V | Write_Z,


		// 周期相关
		ExtraCycle = 0x10000000,    // 需要额外的周期
		// 非官方指令
		Illegal = 0x20000000,
	};


	// 操作码条目
	struct OpcodeEntry
	{
		// 无效(错误)的指令条目
		static const OpcodeEntry None;

		// 指令的操作码
		Opcode opcode;
		// 指令的寻址模式
		AddrMode addrMode;
		// 指令的字节数
		int length;
		// 指令的周期数
		int cycles;
		// 指令分类
		uint32_t kind;

		OpcodeEntry(Opcode opcode, AddrMode addrMode, int cycles, bool extraCycle = false);
	};

	// 根据字节获取操作码条目
	const OpcodeEntry& GetOpcodeEntry(int code);

	// PPU 寄存器对应的地址
	enum PPURegister
	{
		PPU_CTRL = 0x2000,  // 控制寄存器
		PPU_MASK = 0x2001,  // 掩码寄存器
		PPU_STATUS = 0x2002, // 状态寄存器
		OAM_ADDR = 0x2003,  // 精灵属性地址寄存器
		OAM_DATA = 0x2004,  // 精灵属性数据寄存器
		PPU_SCROLL = 0x2005,  // 滚动寄存器
		PPU_ADDR = 0x2006,  // PPU 地址寄存器
		PPU_DATA = 0x2007,  // PPU 数据寄存器
	};
	// 获取 PPU 寄存器地址对应的字符串
	const TCHAR* ToString(PPURegister value);
	// 判断 CPU 地址是不是对应 PPU 寄存器
	inline bool IsAddrOfPPURegister(Address addr)
	{
		return addr >= PPU_CTRL && addr <= PPU_DATA;
	}

	// PPU 控制寄存器
// 8 位
	union PPUControl
	{
		uint8_t value;
		struct
		{
			// 选取 NameTable，00-0x2000, 01-0x2400, 10-0x2800, 11-0x2C00
			uint8_t nameTableSelect : 2;
			// 每次访问 VRAM，地址的增量，0：增长 1 即水平移动，1：增长 32 即纵向移动
			uint8_t increment : 1;
			// 精灵使用哪个 PatternTable，0：0x0000, 1：0x1000
			uint8_t spritePatternTable : 1;
			// 背景使用哪个 PatternTable，0：0x0000, 1：0x1000
			uint8_t backPatternTable : 1;
			// 精灵大小，0：8 * 8，1：8 * 16
			uint8_t spriteSize : 1;
			uint8_t : 1;
			// 是否在 V_Blank 开始的时候产生 NMI
			uint8_t genNMI : 1;
		};
	};

	// PPU 掩码寄存器
	// 8 位
	union PPUMask
	{
		uint8_t value;
		struct
		{
			// bit0，0：显示正常颜色，1：显示黑白图像
			uint8_t greyscale : 1;
			// bit1，1：渲染背景最左侧 8 列像素，0：不渲染
			uint8_t showBackLeft8px : 1;
			// bit2，当精灵位于屏幕最左侧时，1：渲染精灵左侧 8 列像素，0：不渲染
			uint8_t showSpriteLeft8px : 1;
			// bit3，1：渲染背景，0：不渲染
			uint8_t showBackground : 1;
			// bit4，1：渲染精灵，0：不渲染
			uint8_t showSprite : 1;
			// bit5，1：强调红色，0：不强调
			uint8_t eEmphasizeRed : 1;
			// bit6，1：强调绿色，0：不强调
			uint8_t eEmphasizeGreen : 1;
			// bit7，1：强调蓝色，0：不强调
			uint8_t eEmphasizeBlue : 1;
		};
	};

	// PPU 状态寄存器
	// 8 位
	union PPUStatus
	{
		uint8_t value;
		struct
		{
			uint8_t unused : 5;  // 不关心
			// bit5：精灵是否溢出，精灵溢出是只当前扫描行有没有超过 8 个精灵，超过则该位置 1，表溢出
			uint8_t spriteOverflow : 1;
			// bit6：sprite 0 hit，当 sprite 0 的不透明像素与背景不透明像素重叠时该位置 1，
			//			这个主要用于屏幕分割，就是制造那大片级的效果
			uint8_t sprite0Hit : 1;
			// bit7：是否处于 V_Blank，是的话置 1
			uint8_t vBlank : 1;
		};
	};

	// PPU 对CPU公开的寄存器
	union PPURegisters
	{
		uint8_t values[8];
		struct
		{
			PPUControl ctrl;
			PPUMask mask;
			PPUStatus status;
			uint8_t oamAddr;
			uint8_t oamData;
			uint8_t scroll;
			uint8_t addr;
			uint8_t data;
		};
	};

	// PPU 内部不公开寄存器
	struct PPUInternalRegisters
	{
		// 15 位，当前要访问的 VRAM 地址
		// 与滚动寄存器及地址寄存器有关
		// (1) 如果代码滚动，则低12位选择了一个名称表中的一个图块，
		// 高 3 位与 x 决定了该图块的那个像素位于画面左上角
		// (2) 如果代表地址，则低14位为PPU地址
		union
		{
			uint16_t value;
			struct
			{
				uint16_t tileAddr : 12;  // 名称表索引加上图块索引构成的图块地址
			};
			struct
			{
				uint16_t coarseX : 5;  // 滚屏时图块水平索引
				uint16_t coarseY : 5;  // 滚屏时图块垂直索引
				uint16_t nameTable : 2;  // 使用的名称表
				uint16_t fineY : 3;			// 滚屏时图块内像素垂直索引
			};
		}t, v;
		// 3位，存放滚屏时图块内像素的水平索引
		uint8_t x;
		// 1bit，一个开关，因为地址有 16bit，数据总线只有 8bit，所以写地址需要连续写两次，
		// 因此需要一个 toggle 来记录是第一次写还是第二次写。
		uint8_t w;
	};

	// 精灵属性
	union Sprite
	{
		uint32_t value;
		uint8_t bytes[4];
		struct
		{
			uint8_t y;  // y 坐标 (像素)
			uint8_t tile;  // 精灵使用的图块索引
			union
			{
				uint8_t value;
				struct
				{
					// bit0 - 1：该精灵使用的调色板
					uint8_t pallete : 2;
					// bit2 - 4：未使用
					uint8_t unuse : 3;
					// bit5：精灵与背景的优先级，0 表示该精灵在背景前面，1 表示该精灵在背景后面。
					uint8_t priority : 1;
					// bit6：水平翻转
					uint8_t isFlipX : 1;
					// bit7：垂直翻转
					uint8_t isFlipY : 1;
				};
			}attribute;
			uint8_t x;  // x 坐标 (像素)
		};
	};

	// 图案表中的图块
	// 占 16 个字节
	union Pattern
	{
		uint8_t low[8];
		uint8_t high[8];
		// 获取像素索引, 索引的值为 0, 1, 2, 3 中的一个
		int GetPixel(int x, int y)
		{
			x &= 7;
			y &= 7;
			int lowBit = (low[y] >> (7 - x)) & 1;
			int highBit = (high[y] >> (7 - x)) & 1;
			return (highBit << 1) | lowBit;
		}
	};

	// 图案表结构
	// 占 4096 个字节
	struct PatternTable
	{
		Pattern patterns[256];
	};

	// nes ROM 头部
	// 16 个字节
	struct Header
	{
		char signature[4];  // 签名 "NES\x1A"
		uint8_t prgCount;  // 程序块数量，每块 16KB
		uint8_t chrCount;  // 程序块数量，每块 16KB
		union
		{
			uint8_t byte6;
			struct
			{
				uint8_t mirror : 1;  // 0 水平镜像，1 垂直镜像
				uint8_t sram : 1;  // 是否有 SRAM
				uint8_t trainer : 1;  // 是否有 Tranier
				uint8_t screen4 : 1;  // 是否是 4 屏模式
				uint8_t lowMapper : 4;  // mapper 低 4 位
			};
		};
		union
		{
			uint8_t byte7;
			struct
			{
				uint8_t lowBit4 : 4;  // 不关心
				uint8_t highMapper : 4;  // mapper 高 4 位
			};
		};
		uint8_t highByte8[8];  // 高 8 字节，不关心
	};

	// NES CPU 的寄存器枚举
	// A, X, Y 必须是 0， 1， 2，因为TACOptimize中用来索引数组
	enum NesRegisters
	{
		A,
		X,
		Y,
		P,  // 
		SP,
	};
	const TCHAR* ToString(NesRegisters reg);
}
