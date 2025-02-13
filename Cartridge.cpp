#include "stdafx.h"
#include "Cartridge.h"
#include "Mappers.h"
using namespace Nes;

Cartridge::Cartridge() :
correct(0),
prgCount(0),
chrCount(0),
prgOffset(0),
chrOffset(0),
mapperNumber(0),
alignment(Alignment::Horizontal),
hasTrainer(0),
is4Screen(0),
hasSRAM(0)
{
}

Cartridge::~Cartridge()
{
}


void Cartridge::LoadRom(const TCHAR* filename)
{
	FILE* fp;
	_tfopen_s(&fp, filename, _T("rb"));
	if (!fp)
	{
		throw Exception(_T("�� ROM �ļ�ʧ��"));
	}

	fread_s(header, 16, 1, 16, fp);
	if (!Initialize(header))
	{
		throw Exception(_T("��Ч�� NES �ļ�"));
	}

	// ���Ŷ�ȡ����
	uint32_t size = prgCount * 16 * 1024 + chrCount * 8 * 1024;
	if (hasTrainer)
		size += 512;
	data.resize(size);
	fread_s(data.data(), data.size(), 1, size, fp);
	fclose(fp);
}

const uint8_t* Cartridge::GetData(Nes::Address address)
{
	return mapper->GetData(address);
}

bool Cartridge::Initialize(uint8_t* header)
{
	if (!CheckSignature(*(uint32_t*)header))
		return false;
	// 4: byte      ��16384(0x4000)�ֽ���Ϊ��λ��PRG-ROM��С����
	prgCount = header[4];
	// 5: byte      �� 8192(0x2000)�ֽ���Ϊ��λ��CHR-ROM��С����
	chrCount = header[5];
	// 6: byte
	// 7       0
	// ---------
	// NNNN FTBM
	// 
	// N: CMapper��ŵ�4λ
	// F: 4����־λ. (�����λ������, �����M��־)
	// T: Trainer��־λ.  1��ʾ $7000-$71FF���� Trainer
	// B: SRAM��־λ $6000-$7FFFӵ�е�ع����SRAM.
	// M: �����־λ.  0 = ˮƽ, 1 = ��ֱ.
	// 
	// Byte 7 (Flags 7):
	// 7       0
	// ---------
	// NNNN xxPV
	// 
	// N: CMapper��Ÿ�4λ
	// P: Playchoice 10��־λ. ���������ʾΪPC-10��Ϸ
	// V: Vs. Unisystem��־λ. ���������ʾΪVs.  ��Ϸ
	// x: δʹ��
	mapperNumber = ((header[6] & 0xF0) >> 4) | (header[7] & 0xF0);
	hasTrainer = (header[6] & 0x4) != 0;
	is4Screen = (header[6] & 0x8) != 0;
	hasSRAM = (header[6] & 0x2) != 0;
	alignment = (header[6] & 0x1) != 0 ? Alignment::Vertical : Alignment::Horizontal;
	prgOffset = hasTrainer ? 16 + 512 : 16;
	chrOffset = prgOffset + prgCount * 16 * 1024;
	mapper = ::GetMapper(*this);
	return true;
}
