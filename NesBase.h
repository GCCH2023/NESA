#pragma once

namespace Nes
{
	// ��ַ����
	using Address = uint16_t;


	// ��Ϸ�����з�ʽ
	enum Alignment
	{
		Horizontal = 0,  // ˮƽ
		Vertical = 1,  // ��ֱ
	};

	// �ֽڸ�λ�ı�־
	enum Flags
	{
		C = 1 << 0,    // ��λ (Carry flag)
		Z = 1 << 1,    // �� (Zero flag)
		I = 1 << 2,    // ��ֹ�ж� (Irq disabled flag)
		D = 1 << 3,    // ʮ����ģʽ (Decimal mode flag)
		B = 1 << 4,    // ����ж� (BRK flag)
		U = 1 << 5,    // ���� (Unuse), һֱΪ1
		V = 1 << 6,    // ��� (Overflow  flag)
		N = 1 << 7,    // ���� (Negative flag)
	};

	// ״̬��־λ
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

	// �ж�����
	enum InterruptVector
	{
		NMI = 0xFFFA,
		RESET = 0xFFFC,
		IRQ = 0xFFFE
	};

	// Ѱַģʽ
	enum AddrMode
	{
		// ��ʽѰַ [��ʽ] Implied Addressing [Implied]
		// ����ʽѰַģʽ� �����������ĵ�ַ��������ָ��Ĳ������
		Implied,
		// �ۼ���Ѱַ [�ۼ���] Accumulator Addressing [Accumulator]
		// ����Ѱַģʽ����Ϊֻ��һ������������һ������������Ϊ�ۼ�����
		Accumulator,
		// ֱ��Ѱַ [ֱ��] Immediate Addressing [Immediate]
		// ��ֱ��Ѱַģʽ���������������ָ��ĵڶ����ֽ������Ҫ�����ڴ�Ѱַ��
		Immediate,
		// 0ҳ��Ѱַ [0ҳ��] Zero Page Addressing [Zero Page]
		// 0ҳ��Ѱַ����ֻȡָ��ĵڶ��ֽڲ������λ��ַΪ0���Եõ��϶̵Ĵ����ִ��ʱ�䡣
		// С�ĵ�ʹ��0ҳ��Ѱַ����Ч�����Ӵ����Ч�ʡ�
		ZeroPage,
		// 0ҳ���ַѰַ [0ҳ��,X �� 0ҳ��,Y] Zero Page Indexed Addressing [Zero Page,X or Zero Page,Y]
		// 0ҳ���ַѰַ����X��Y��ַ�Ĵ���ʹ�ã��ԡ�0ҳ��, X���͡�0ҳ��, Y������ʽ���֡�
		// ��Ч��ַ��ָ��ĵڶ����ֽڼ��ϱ�ַ�Ĵ��������ݵõ���
		// ��Ϊ����һ����0ҳ�桱Ѱַ�����Եڶ��ֽڵ����ݶ�λ��0ҳ�档
		// ���⣬��Ϊ��0ҳ�桱Ѱַģʽ�����ԣ����Բ����н�λ�ӵ���λ�ڴ棬Ҳ���ᷢ����ҳ�߽�����⡣
		ZeroPageX,
		ZeroPageY,
		// ���Ѱַ [���] Relative Addressing [Relative]
		// ���Ѱַֻ����ת�ƣ�branch��ָ��ָ���У���ָ��������ת�Ƶ�Ŀ�ꡣ
		// ָ��ĵڶ����ֽڱ��һ������������Ϊƫ�Ƽӵ�ָ����һ��ָ���ָ��ָ���ϡ�
		// ƫ�Ʒ�Χ��-128��127�ֽڣ��������һ��ָ�
		Relative,
		// ����Ѱַ [����] Absolute Addressing [Absolute]
		// �ھ���Ѱַģʽ�ָ��ĵڶ����ֽ�ָ������Ч��ַ�ĵͰ�λ���������ֽ�ָ���߰�λ��
		// ��ˣ�����Ѱַģʽ�������ȫ����64K��Ѱַ�ڴ档
		Absolute,
		// ���Ա�ַѰַ [����,X �� ����,Y] Absolute Indexed Addressing [Absolute,X or Absolute,Y]
		// ���Ա�ַѰַ����X��Y��ַ�Ĵ���ʹ�ã��ԡ�����, X���͡�����, Y������ʽ���֡�
		// ��Ч��ַ��X��Y�����ݼ���ָ��ĵڶ�������ֽڰ����ĵ�ַ�õ���
		// ����ģʽ�����ַ�Ĵ���������ַ�����ֵ��ָ�������ַ��
		// ����Ѱַģʽ����λ�κ�λ�ò��ҿ����ñ�ַ�޸Ķ���ط������Լ��ٴ����ָ��ʱ�䡣
		AbsoluteX,
		AbsoluteY,
		Indirect,
		// 0ҳ���ַ���Ѱַ [(0ҳ��,X)] Zero Page Indexed Indirect Addressing [(Zero Page,X)
		// 
		// 0ҳ���ַ���Ѱַ��ͨ���ο�Ϊ ��� X��ָ��ĵڶ����ֽڼӵ�X��ַ�Ĵ����������ϣ���λ��������
		// �ӷ�����Ľ��ָ��0ҳ���һ���ڴ浥Ԫ��������ڴ浥Ԫ����Ч��ַ�ĵ�8λ��
		// ��һ��0ҳ�浥Ԫ����Ч��ַ�ĸ�8λ��ָ����Ч��ַ�ĸ�8λ�͵�8λ��������0ҳ���ڡ�
		IndirectX,
		// [Y + [M]]
		IndirectY,
	};

	// ����Ѱַģʽ��ȡָ����ֽ���
	int GetInstructionBytesCount(AddrMode addrMode);
	const TCHAR* ToString(AddrMode addrMode);

	// NES ������ö��
	enum class Opcode
	{
		None,  // δ����
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

	// ��ȡ��������ַ�����ʾ
	const TCHAR* ToString(Opcode opcode);

	// �������Ѱַģʽö��
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

	// ָ��ķ���
	// �������ֿ���
	enum OpKind
	{
		None = 0,       // ������
		// ��һ����, �����ܻ���
		Logical = 0x1,        // �߼�
		Arithmetic = 0x2,     // ����
		LogicalArithmetic = 0x3,    // �߼�������
		Move = 0x4,           // ����
		ConditionalJump = 0x8,    // ������ת
		UnconditionalJump = 0x10,     // ��������ת
		Call = 0x20,            // ��������
		Return = 0x40,        // ��������
		Jump = ConditionalJump | UnconditionalJump | Call | Return,     // ��תָ��
		// �ڶ�����, �Ƿ��Ӱ��ָ���Ĵ���
		ReadA = 0x100,  // ��ȡ�Ĵ���A
		WriteA = 0x200,  // д��Ĵ���A
		RWa = 0x300,    // ��ȡ��д��Ĵ���A
		ReadX = 0x400,  // ��ȡ�Ĵ���X
		WriteX = 0x800,  // д��Ĵ���X
		RWx = 0xC00,    // ��ȡ��д��Ĵ���X
		ReadY = 0x1000,  // ��ȡ�Ĵ���Y
		WriteY = 0x2000,  // д��Ĵ���Y
		RWy = 0x3000,    // ��ȡ��д��Ĵ���Y
		ReadPC = 0x4000,  // ��ȡ�Ĵ���PC
		WritePC = 0x8000,  // д��Ĵ���PC
		RWpc = 0xC000,    // ��ȡ��д��Ĵ���PC
		ReadSP = 0x10000,  // ��ȡ�Ĵ���SP
		WriteSP = 0x20000,  // д��Ĵ���SP
		RWsp = 0x30000,    // ��ȡ��д��Ĵ���SP
		ReadM = 0x10000,  // ��ȡ�ڴ�
		WriteM = 0x20000,  // д���ڴ�
		RWm = 0x30000,    // ��ȡ��д���ڴ�
		// ״̬�Ĵ���
		Write_C = 0x100000,     // Ӱ���λ��־
		Write_Z = 0x200000,     // Ӱ�����־
		Write_I = 0x400000,     // Ӱ���ж����α�־
		Write_D3 = 0x800000,
		Write_B = 0x1000000,
		Write_D5 = 0x1000000,
		Write_V = 0x4000000,     // Ӱ�������־
		Write_N = 0x8000000,     // Ӱ�츺����־
		ReadP = 0x10000000,     // ��ȡ��־�Ĵ���
		WriteP = 0xF00000,      // д���־�Ĵ���
		RWp = 0x1F00000,     // Ӱ���־�Ĵ���

		NZ = Write_N | Write_Z,
		BI = Write_B | Write_I,
		NZC = Write_N | Write_Z | Write_C,
		NVZC = NZC | Write_V,
		NVZ = Write_N | Write_V | Write_Z,


		// �������
		ExtraCycle = 0x10000000,    // ��Ҫ���������
		// �ǹٷ�ָ��
		Illegal = 0x20000000,
	};


	// ��������Ŀ
	struct OpcodeEntry
	{
		// ��Ч(����)��ָ����Ŀ
		static const OpcodeEntry None;

		// ָ��Ĳ�����
		Opcode opcode;
		// ָ���Ѱַģʽ
		AddrMode addrMode;
		// ָ����ֽ���
		int length;
		// ָ���������
		int cycles;
		// ָ�����
		uint32_t kind;

		OpcodeEntry(Opcode opcode, AddrMode addrMode, int cycles, bool extraCycle = false);
	};

	// �����ֽڻ�ȡ��������Ŀ
	const OpcodeEntry& GetOpcodeEntry(int code);

	// PPU �Ĵ�����Ӧ�ĵ�ַ
	enum PPURegister
	{
		PPU_CTRL = 0x2000,  // ���ƼĴ���
		PPU_MASK = 0x2001,  // ����Ĵ���
		PPU_STATUS = 0x2002, // ״̬�Ĵ���
		OAM_ADDR = 0x2003,  // �������Ե�ַ�Ĵ���
		OAM_DATA = 0x2004,  // �����������ݼĴ���
		PPU_SCROLL = 0x2005,  // �����Ĵ���
		PPU_ADDR = 0x2006,  // PPU ��ַ�Ĵ���
		PPU_DATA = 0x2007,  // PPU ���ݼĴ���
	};
	// ��ȡ PPU �Ĵ�����ַ��Ӧ���ַ���
	const TCHAR* ToString(PPURegister value);
	// �ж� CPU ��ַ�ǲ��Ƕ�Ӧ PPU �Ĵ���
	inline bool IsAddrOfPPURegister(Address addr)
	{
		return addr >= PPU_CTRL && addr <= PPU_DATA;
	}

	// PPU ���ƼĴ���
// 8 λ
	union PPUControl
	{
		uint8_t value;
		struct
		{
			// ѡȡ NameTable��00-0x2000, 01-0x2400, 10-0x2800, 11-0x2C00
			uint8_t nameTableSelect : 2;
			// ÿ�η��� VRAM����ַ��������0������ 1 ��ˮƽ�ƶ���1������ 32 �������ƶ�
			uint8_t increment : 1;
			// ����ʹ���ĸ� PatternTable��0��0x0000, 1��0x1000
			uint8_t spritePatternTable : 1;
			// ����ʹ���ĸ� PatternTable��0��0x0000, 1��0x1000
			uint8_t backPatternTable : 1;
			// �����С��0��8 * 8��1��8 * 16
			uint8_t spriteSize : 1;
			uint8_t : 1;
			// �Ƿ��� V_Blank ��ʼ��ʱ����� NMI
			uint8_t genNMI : 1;
		};
	};

	// PPU ����Ĵ���
	// 8 λ
	union PPUMask
	{
		uint8_t value;
		struct
		{
			// bit0��0����ʾ������ɫ��1����ʾ�ڰ�ͼ��
			uint8_t greyscale : 1;
			// bit1��1����Ⱦ��������� 8 �����أ�0������Ⱦ
			uint8_t showBackLeft8px : 1;
			// bit2��������λ����Ļ�����ʱ��1����Ⱦ������� 8 �����أ�0������Ⱦ
			uint8_t showSpriteLeft8px : 1;
			// bit3��1����Ⱦ������0������Ⱦ
			uint8_t showBackground : 1;
			// bit4��1����Ⱦ���飬0������Ⱦ
			uint8_t showSprite : 1;
			// bit5��1��ǿ����ɫ��0����ǿ��
			uint8_t eEmphasizeRed : 1;
			// bit6��1��ǿ����ɫ��0����ǿ��
			uint8_t eEmphasizeGreen : 1;
			// bit7��1��ǿ����ɫ��0����ǿ��
			uint8_t eEmphasizeBlue : 1;
		};
	};

	// PPU ״̬�Ĵ���
	// 8 λ
	union PPUStatus
	{
		uint8_t value;
		struct
		{
			uint8_t unused : 5;  // ������
			// bit5�������Ƿ���������������ֻ��ǰɨ������û�г��� 8 �����飬�������λ�� 1�������
			uint8_t spriteOverflow : 1;
			// bit6��sprite 0 hit���� sprite 0 �Ĳ�͸�������뱳����͸�������ص�ʱ��λ�� 1��
			//			�����Ҫ������Ļ�ָ���������Ǵ�Ƭ����Ч��
			uint8_t sprite0Hit : 1;
			// bit7���Ƿ��� V_Blank���ǵĻ��� 1
			uint8_t vBlank : 1;
		};
	};

	// PPU ��CPU�����ļĴ���
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

	// PPU �ڲ��������Ĵ���
	struct PPUInternalRegisters
	{
		// 15 λ����ǰҪ���ʵ� VRAM ��ַ
		// ������Ĵ�������ַ�Ĵ����й�
		// (1) ���������������12λѡ����һ�����Ʊ��е�һ��ͼ�飬
		// �� 3 λ�� x �����˸�ͼ����Ǹ�����λ�ڻ������Ͻ�
		// (2) ��������ַ�����14λΪPPU��ַ
		union
		{
			uint16_t value;
			struct
			{
				uint16_t tileAddr : 12;  // ���Ʊ���������ͼ���������ɵ�ͼ���ַ
			};
			struct
			{
				uint16_t coarseX : 5;  // ����ʱͼ��ˮƽ����
				uint16_t coarseY : 5;  // ����ʱͼ�鴹ֱ����
				uint16_t nameTable : 2;  // ʹ�õ����Ʊ�
				uint16_t fineY : 3;			// ����ʱͼ�������ش�ֱ����
			};
		}t, v;
		// 3λ����Ź���ʱͼ�������ص�ˮƽ����
		uint8_t x;
		// 1bit��һ�����أ���Ϊ��ַ�� 16bit����������ֻ�� 8bit������д��ַ��Ҫ����д���Σ�
		// �����Ҫһ�� toggle ����¼�ǵ�һ��д���ǵڶ���д��
		uint8_t w;
	};

	// ��������
	union Sprite
	{
		uint32_t value;
		uint8_t bytes[4];
		struct
		{
			uint8_t y;  // y ���� (����)
			uint8_t tile;  // ����ʹ�õ�ͼ������
			union
			{
				uint8_t value;
				struct
				{
					// bit0 - 1���þ���ʹ�õĵ�ɫ��
					uint8_t pallete : 2;
					// bit2 - 4��δʹ��
					uint8_t unuse : 3;
					// bit5�������뱳�������ȼ���0 ��ʾ�þ����ڱ���ǰ�棬1 ��ʾ�þ����ڱ������档
					uint8_t priority : 1;
					// bit6��ˮƽ��ת
					uint8_t isFlipX : 1;
					// bit7����ֱ��ת
					uint8_t isFlipY : 1;
				};
			}attribute;
			uint8_t x;  // x ���� (����)
		};
	};

	// ͼ�����е�ͼ��
	// ռ 16 ���ֽ�
	union Pattern
	{
		uint8_t low[8];
		uint8_t high[8];
		// ��ȡ��������, ������ֵΪ 0, 1, 2, 3 �е�һ��
		int GetPixel(int x, int y)
		{
			x &= 7;
			y &= 7;
			int lowBit = (low[y] >> (7 - x)) & 1;
			int highBit = (high[y] >> (7 - x)) & 1;
			return (highBit << 1) | lowBit;
		}
	};

	// ͼ����ṹ
	// ռ 4096 ���ֽ�
	struct PatternTable
	{
		Pattern patterns[256];
	};

	// nes ROM ͷ��
	// 16 ���ֽ�
	struct Header
	{
		char signature[4];  // ǩ�� "NES\x1A"
		uint8_t prgCount;  // �����������ÿ�� 16KB
		uint8_t chrCount;  // �����������ÿ�� 16KB
		union
		{
			uint8_t byte6;
			struct
			{
				uint8_t mirror : 1;  // 0 ˮƽ����1 ��ֱ����
				uint8_t sram : 1;  // �Ƿ��� SRAM
				uint8_t trainer : 1;  // �Ƿ��� Tranier
				uint8_t screen4 : 1;  // �Ƿ��� 4 ��ģʽ
				uint8_t lowMapper : 4;  // mapper �� 4 λ
			};
		};
		union
		{
			uint8_t byte7;
			struct
			{
				uint8_t lowBit4 : 4;  // ������
				uint8_t highMapper : 4;  // mapper �� 4 λ
			};
		};
		uint8_t highByte8[8];  // �� 8 �ֽڣ�������
	};

	// NES CPU �ļĴ���ö��
	// A, X, Y ������ 0�� 1�� 2����ΪTACOptimize��������������
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
