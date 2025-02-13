#pragma once
#include "Mapper.h"

class Cartridge
{
public:
	// 检查 nes 头部标志 "NES\x1A"
	static bool CheckSignature(uint32_t value)
	{
		return value == *(uint32_t*)"NES\x1A";
	}

	Cartridge();
	virtual ~Cartridge();

	// 加载 ROM 文件
	void LoadRom(const TCHAR* filename);

	// 获取文件格式是否正确
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

	// 读取ROM的一个字节
	uint8_t RawReadByte(int index) const
	{
		return data[index - 16];
	}
	// 读取ROM的 2 个字节
	uint16_t RawReadWord(int offset) const
	{
		offset -= 16;
		return *(uint16_t*)(data.data() + offset);
	}
	// 获取数据指针
	const uint8_t* RawGetData(int offset) const
	{
		offset -= 16;
		return data.data() + offset;
	}
	CMapper* GetMapper() { return mapper.get(); }
	uint8_t* GetHeader() { return header; }

	// 根据地址获取数据指针
	const uint8_t* GetData(Nes::Address address);
protected:
	// 初始化字段，必须先序列化
	// 返回是否成功，失败则文件格式错误
	bool Initialize(uint8_t* header);
private:
	bool correct;  // 文件格式是否正确
	int prgCount;
	int chrCount;
	int prgOffset;
	int chrOffset;
	int mapperNumber;
	Nes::Alignment alignment;
	bool hasTrainer;
	bool is4Screen;
	bool hasSRAM;
	uint8_t header[16]; // 16 字节的头部
	std::vector<uint8_t> data; // 除了头部的其他数据
	std::shared_ptr<CMapper> mapper;
};
