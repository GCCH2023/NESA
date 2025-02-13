#pragma once
#include "Mapper.h"

// 0 ��ӳ����
// ���򲿷�ֻ������32kb��16kb��ͼ�񲿷���8kb
// ������򲿷���32kb����ӳ�䵽 0x8000 - 0x10000
// ������򲿷���16kb����ӳ�䵽 0x8000 - 0xA000, ������ 0xA000 - 0x10000
// ͼ�񲿷�ӳ�䵽 PPU 0x0 - 0x2000
class CMapper000 : public CMapper
{
public:
	CMapper000(Cartridge& cartridge);
	// ��ȡ�����ַ����һ���ֽ�
	virtual uint8_t ReadByte(Nes::Address virtualAddress) override;
	virtual uint16_t ReadWord(Nes::Address virtualAddress) override;
	virtual int ToFileOffset(Nes::Address virtualAddress) const override;
};

// ��ȡ������Ӧ��ӳ����
std::shared_ptr<CMapper> GetMapper(Cartridge& cartridge);
