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
		throw Exception(_T("打开 ROM 文件失败"));
	}

	fread_s(header, 16, 1, 16, fp);
	if (!Initialize(header))
	{
		throw Exception(_T("无效的 NES 文件"));
	}

	// 接着读取数据
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
	// 4: byte      以16384(0x4000)字节作为单位的PRG-ROM大小数量
	prgCount = header[4];
	// 5: byte      以 8192(0x2000)字节作为单位的CHR-ROM大小数量
	chrCount = header[5];
	// 6: byte
	// 7       0
	// ---------
	// NNNN FTBM
	// 
	// N: CMapper编号低4位
	// F: 4屏标志位. (如果该位被设置, 则忽略M标志)
	// T: Trainer标志位.  1表示 $7000-$71FF加载 Trainer
	// B: SRAM标志位 $6000-$7FFF拥有电池供电的SRAM.
	// M: 镜像标志位.  0 = 水平, 1 = 垂直.
	// 
	// Byte 7 (Flags 7):
	// 7       0
	// ---------
	// NNNN xxPV
	// 
	// N: CMapper编号高4位
	// P: Playchoice 10标志位. 被设置则表示为PC-10游戏
	// V: Vs. Unisystem标志位. 被设置则表示为Vs.  游戏
	// x: 未使用
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
