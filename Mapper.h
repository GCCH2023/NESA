#pragma once

class Cartridge;
// ӳ����
class CMapper
{
public:
	CMapper(Cartridge& cartridge);
	// ��ָ�������ַ����ȡ1���ֽ�
	virtual uint8_t ReadByte(Nes::Address virtualAddress) = 0;
	// ��ָ�������ַ����ȡ2���ֽ�
	virtual uint16_t ReadWord(Nes::Address virtualAddress) = 0;
	// ��ָ�������ַת��Ϊ ROM �ļ�ƫ����
	// !!! ͬһ�������ַ�����ڲ�ͬʱ�̶�Ӧ��ͬ�� �ļ�ƫ����
	virtual int ToFileOffset(Nes::Address virtualAddress) const = 0;
	const uint8_t* GetData(Nes::Address address) const;
protected:
	Cartridge& cartridge;
};
