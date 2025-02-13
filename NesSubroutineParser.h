#pragma once
class NesDataBase;
struct Instruction;
class NesSubroutine;
class NesBasicBlock;
struct CallRelation;

// ���� NES �ӳ���
class NesSubroutineParser
{
public:
	NesSubroutineParser(NesDataBase& db);
	~NesSubroutineParser();

	// ��ָ����ַ����Ϊ�ӳ���
	virtual NesSubroutine* Parse(Nes::Address address);
	// ���ý������������Ϳ�����һ����������������ν����ӳ�����
	void Reset();
	// ����ӳ������Ϣ
	void Dump();
protected:
	// ����Ҫ����һ��ָ���ʱ����ã�����ֵ��ʾ�Ƿ����������һ��ָ��
	virtual bool ParseInstruction(const Instruction& instruction);
	// ���ָ����ַ��Ϊ�����鿪ʼ��ַ ����С�������в���ȥ�أ�
	void AddBasicBlockStartAddress(Nes::Address address);
	// �жϵ�ַ�Ƿ��ǻ����鿪ʼ��ַ
	bool IsBlockStartAddress(Nes::Address address);
	// ��֤�������ַ�Ƿ��ں����ڲ�
	bool CheckBlocksAddress(Nes::Address start, Nes::Address end);
	// ����������֮������ӹ�ϵ
	void ParseBasicBlocks();
	// ���������������ָ��
	void ParseBasicBlockInstructions(NesBasicBlock* block);
	// ����������ĵ���ָ��
	// nextAddr �Ǻ���һ��ָ��ĵ�ַ
	void ParseBasicBlockInstruction(NesBasicBlock* block, const Instruction& instruction, Nes::Address nextAddr);
	// ���������������ǰ���ϵ
	void BindBlock(NesBasicBlock* prev, Nes::Address nextAddr);
	// ���û��������ת��ر�־
	void SetBasickBlockJumpFlag(NesBasicBlock* block, bool isCond, Nes::Address jumpAddr);
	// ���汻�����ӳ���ĵ�ַ
	inline void AddCalledAddress(Nes::Address calledAddr) { calls.push_back(calledAddr); }
protected:
	NesDataBase& db;
	std::vector<Nes::Address> blockStartAddrs;  // �����鿪ʼ��ַ�б�
	NesSubroutine* subroutine;
	std::vector<Nes::Address> calls;
};

