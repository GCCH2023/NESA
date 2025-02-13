#pragma once
#include "Mapper.h"

class Cartridge
{
public:
	// ��� nes ͷ����־ "NES\x1A"
	static bool CheckSignature(uint32_t value)
	{
		return value == *(uint32_t*)"NES\x1A";
	}

	Cartridge();
	virtual ~Cartridge();

	// ���� ROM �ļ�
	void LoadRom(const TCHAR* filename);

	// ��ȡ�ļ���ʽ�Ƿ���ȷ
	bool IsCorrect() const { return correct; }
	inline int GetPRGCount() const { return prgCount; }
	inline int GetPRGOffset() const { return prgOffset; }
	inline int GetCHRCount() const { return chrCount; }
	inline int GetCHROffset() const { return chrOffset; }
	inline int GetMapperNumber() const { return mapperNumber; }
	inline bool HasTrainer() const { return hasTrainer; }
	inline bool HasSRAM() const { return hasSRAM; }
	inline bool Is4Screen() const { return is4Screen; }
	inline Nes::Alignment GetAlignment() const { return alignment; }
	inline size_t GetSize() const { return data.size() + 16; }

	inline uint8_t ReadByte(Nes::Address address)
	{
		return mapper->ReadByte(address);
	}
	inline uint16_t ReadWord(Nes::Address address)
	{
		return mapper->ReadWord(address);
	}

	// ��ȡROM��һ���ֽ�
	uint8_t RawReadByte(int index) const
	{
		return data[index - 16];
	}
	// ��ȡROM�� 2 ���ֽ�
	uint16_t RawReadWord(int offset) const
	{
		offset -= 16;
		return *(uint16_t*)(data.data() + offset);
	}
	// ��ȡ����ָ��
	const uint8_t* RawGetData(int offset) const
	{
		offset -= 16;
		return data.data() + offset;
	}
	CMapper* GetMapper() { return mapper.get(); }
	uint8_t* GetHeader() { return header; }

	// ���ݵ�ַ��ȡ����ָ��
	const uint8_t* GetData(Nes::Address address);
protected:
	// ��ʼ���ֶΣ����������л�
	// �����Ƿ�ɹ���ʧ�����ļ���ʽ����
	bool Initialize(uint8_t* header);
private:
	bool correct;  // �ļ���ʽ�Ƿ���ȷ
	int prgCount;
	int chrCount;
	int prgOffset;
	int chrOffset;
	int mapperNumber;
	Nes::Alignment alignment;
	bool hasTrainer;
	bool is4Screen;
	bool hasSRAM;
	uint8_t header[16]; // 16 �ֽڵ�ͷ��
	std::vector<uint8_t> data; // ����ͷ������������
	std::shared_ptr<CMapper> mapper;
};
