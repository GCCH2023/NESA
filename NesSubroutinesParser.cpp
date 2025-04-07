#include "stdafx.h"
#include "NesSubroutinesParser.h"
#include "NesSubroutineParser.h"
#include "NesDataBase.h"
#include "NesUtil.h"

NesSubroutinesParser::NesSubroutinesParser(NesDataBase& db_):
db(db_)
{

}

NesSubroutinesParser::~NesSubroutinesParser()
{
}

std::vector<NesSubroutine*>& NesSubroutinesParser::Parse(Nes::Address address)
{
	NesSubroutineParser parser(db);
	// 构建初始的待分析子程地址序队列
	std::vector<Nes::Address> queue =
	{
		// db.GetInterruptResetAddress(),
		db.GetInterruptNmiAddress(),
		//db.GetInterruptIrqAddress(),
	};
	Sprintf<> s;
	NesSubroutineParser subParser(db);
	// 循环分析所有函数
	while (!queue.empty())
	{
		// 取出一个函数地址进行分析
		auto addr = *queue.rbegin();
		queue.pop_back();

		// parser 会判断子程序是否分析过，所以这里不需要判断
		NesSubroutine* subroutine = ParseSubroutine(addr, subParser);
		AddSubroutine(subroutine);

		// 将子程序调用的子程序添加到队列
		for (auto call : subroutine->GetCalls())
		{
			if (subMap.find(call) == subMap.end())
			{
				// 没有分析过才添加到队列
				queue.push_back(call);
			}
		}
	}
	return subroutines;
}

NesSubroutine* NesSubroutinesParser::ParseSubroutine(Nes::Address address, NesSubroutineParser& subParser)
{
	uint32_t maxAddress = 0x10000;  // 分析范围的上限
	auto dbSub = db.GetSubroutineOrNext(address);
	if (dbSub)
	{
		if (address == dbSub->GetStartAddress())
			return dbSub;  // 已经分析过了
		if (address < dbSub->GetStartAddress())
			maxAddress = dbSub->GetStartAddress();  // 最多到后面一个函数的开始地址
		else if (address < dbSub->GetEndAddress())
		{
			// 当前函数内联在另一个函数中，拆分后重新分析，因为可能有指令从
			// 拆分后的一个子程序跳转到另一个子程序
			maxAddress = dbSub->GetEndAddress();
			dbSub->SetEndAddress(address);
			dbSub->Clear();
			subParser.Parse(dbSub);
		}
	}
	auto subroutine = db.allocator.New<NesSubroutine>(address, maxAddress);
	subParser.Parse(subroutine);
	db.AddSubroutine(subroutine);
	return subroutine;
}

void NesSubroutinesParser::AddSubroutine(NesSubroutine* subroutine)
{
	subMap.insert({ subroutine->GetStartAddress(), subroutine });
	AddNesObject(subroutines, subroutine);
}
