#include "stdafx.h"
#include "ReachingDefinition.h"
#include "TAC.h"
#include "NesDataBase.h"
using namespace std;
#include "TACTranslater.h"




void DumpBasicBlockDefinitions(TACBasicBlock* block, TacAxyDefinition& axyDefs, TACSubroutine* tacSub)
{
	vector<TAC*> codes(32);
	TCHAR buffer[64];
	auto blockSet = (BasicBlockReachingDefinitionSet*)block->tag;
	_stprintf_s(buffer, 64, _T("block %04X:\nIN:\n"), block->GetStartAddress());
	COUT << buffer;
	axyDefs.GetDefinitionTACList(codes, blockSet->in, tacSub);
	for (auto tac : codes)
		DumpAddressTAC(COUT, tac) << endl;
	COUT << "OUT:\n";
	axyDefs.GetDefinitionTACList(codes, blockSet->out, tacSub);
	for (auto tac : codes)
		DumpAddressTAC(COUT, tac) << endl;
}


void DumpAllBasicBlockDefinitions(TacAxyDefinition& axyDefs, TACSubroutine* tacSub)
{
	for (auto block : tacSub->GetBasicBlocks())
	{
		DumpBasicBlockDefinitions(block, axyDefs, tacSub);
	}
}

ReachingDefinition::ReachingDefinition(NesDataBase& db_, Allocator& allocator_):
DataFlowAnalyzer(db_),
allocator(allocator_)
{

}

ReachingDefinition::~ReachingDefinition()
{
}


// ���AXY�����ж�ֵ����ַ��
void DumpAXYDefinitions(TacAxyDefinition& axyDefs, TACSubroutine* tacSub)
{
	auto& codes = tacSub->GetCodes();
	COUT << "��ȡAXY�����ж�ֵ�㣺\n";
	COUT << "A:\n";
	for (auto i : axyDefs.adefs)
	{
		DumpAddressTAC(COUT, codes[i]) << endl;
	}
	COUT << "X:\n";
	for (auto i : axyDefs.xdefs)
	{
		DumpAddressTAC(COUT, codes[i]) << endl;
	}
	COUT << "Y:\n";
	for (auto i : axyDefs.ydefs)
	{
		DumpAddressTAC(COUT, codes[i]) << endl;
	}
}

// ��ȡ����ַ���е����ж�A, X, Y �Ĵ����Ķ�ֵ��
void ReachingDefinition::GetAXYDefinitions(TacAxyDefinition& axyDefs, TACSubroutine* tacSub)
{
	int i = 0;
	for (auto code : tacSub->GetCodes())
	{
		// ��������Ҳ���ܸ�AXY��ֵ
		if (code->op == TACOperator::CALL)
		{
			auto sub = db.FindSubroutine(code->z.GetValue());
			if (sub->flag & SUBF_RTURN_A)
				axyDefs.adefs.push_back(i);
			if (sub->flag & SUBF_RTURN_X)
				axyDefs.xdefs.push_back(i);
			if (sub->flag & SUBF_RTURN_Y)
				axyDefs.ydefs.push_back(i);
			continue;
		}
		if (code->z.IsRegister())
		{
			switch (code->z.GetValue())
			{
			case  Nes::NesRegisters::A:
				axyDefs.adefs.push_back(i);
				break;
			case  Nes::NesRegisters::X:
				axyDefs.xdefs.push_back(i);
				break;
			case  Nes::NesRegisters::Y:
				axyDefs.ydefs.push_back(i);
				break;
			}
		}
		++i;
	}
	//DumpAXYDefinitions(axyDefs, tacSub);
}

void ReachingDefinition::Uninitialize()
{
	// DumpAllBasicBlockDefinitions(axyDefs, this->subroutine);
}

TACBasicBlock* GetBasickBlockByAddress(const TACBasicBlockList& blocks, Nes::Address address)
{
	for (auto block : blocks)
	{
		if (address < block->GetEndAddress())
			return block;
	}
	throw Exception(_T("�Ҳ���ָ����ַ�Ļ�����"));
	// ���ҵ�һ��������ַ����ָ����ַ�Ļ�����
	// !! ��֪������Ĵ�����ʲô���⣬������ǲ���
	/*auto it = std::upper_bound(blocks.begin(), blocks.end(), address,
		[](Nes::Address a, BasicBlock* b) {
		std::COUT << a << " < " << b->GetEndAddress() << " : " << (a < b->GetEndAddress()) << std::endl;
		return a < b->GetEndAddress();
		});
		if (it != blocks.end())
		*it;
		return nullptr;*/
}

// ������������������ɱ����
void GetBasickBlockGenKillMap(TacAxyDefinition& axyDefs, TACSubroutine* tacSub,
	const TACBasicBlockList& blocks)
{
	auto codes = tacSub->GetCodes();
	// ���α��� AXY �Ĵ����Ķ�ֵ�㣬��ȡ��Ӧ�Ļ�������������������ɱ����
	size_t index = 0;
	for (auto d : axyDefs.adefs)
	{
		auto code = codes[d];
		TACBasicBlock* block = GetBasickBlockByAddress(blocks, code->address);
		((BasicBlockReachingDefinitionSet*)block->tag)->genKill.a = 1 << index;  // ���
		++index;
	}
	index = 0;
	for (auto d : axyDefs.xdefs)
	{
		auto code = codes[d];
		TACBasicBlock* block = GetBasickBlockByAddress(blocks, code->address);
		((BasicBlockReachingDefinitionSet*)block->tag)->genKill.x = 1 << index;  // ���
		++index;
	}
	index = 0;
	for (auto d : axyDefs.ydefs)
	{
		auto code = codes[d];
		TACBasicBlock* block = GetBasickBlockByAddress(blocks, code->address);
		((BasicBlockReachingDefinitionSet*)block->tag)->genKill.y = 1 << index;  // ���
		++index;
	}
}

void ReachingDefinition::Initialize()
{
	auto& blocks = this->subroutine->GetBasicBlocks();
	for (auto block : blocks)
	{
		block->tag = allocator.New<BasicBlockReachingDefinitionSet>();
	}

	// ֻ�����Ĵ��� A, X, Y�������Ĵ�������ʱ��������ȫ�ֱ������Ե�
	GetAXYDefinitions(axyDefs, this->subroutine);
	axyDefs.CheckDefinitionLimit();

	// �����������������������ɼ���ɱ����
	GetBasickBlockGenKillMap(axyDefs, this->subroutine, blocks);
}

bool ReachingDefinition::IteraterBasicBlock(TACBasicBlock* block)
{
	auto blockSet = (BasicBlockReachingDefinitionSet*)block->tag;
	// IN[B] = ����ǰ���Ķ�ֵ��Ĳ���
	// OUT[B] = IN[B] �� gen_kill(B)
	for (auto prev : block->prevs)
	{
		auto prevSet = (BasicBlockReachingDefinitionSet*)prev->tag;
		blockSet->in |= prevSet->out;
	}
	return !blockSet->EvalOut();
}


AXYSet& AXYSet::operator|=(const AXYSet& other)
{
	a |= other.a;
	x |= other.x;
	y |= other.y;
	return *this;
}

bool BasicBlockReachingDefinitionSet::EvalOut()
{
	auto value = out.a;
	bool ret = false;
	out.a = genKill.a.Any() ? genKill.a : in.a;
	if (out.a != value)
		ret = true;
	value = out.x;
	out.x = genKill.x.Any() ? genKill.x : in.x;
	if (out.x != value)
		ret = true;
	value = out.y;
	out.y = genKill.y.Any() ? genKill.y : in.y;
	if (out.y != value)
		ret = true;
	return ret;
}

void TacAxyDefinition::CheckDefinitionLimit()
{
	if (adefs.size() > 32 || xdefs.size() > 32 || ydefs.size() > 32)
		throw Exception(_T("���ӳ�����е��ﶨֵ����ʱ��AXY�е�һ����ֵ����������32\n"));
}

void TacAxyDefinition::GetDefinitionTACList(std::vector<TAC*>& result, AXYSet& set, TACSubroutine* tacSub)
{
	result.clear();
	auto& codes = tacSub->GetCodes();
	auto list = set.a.ToVector();
	for (auto i : list)
		result.push_back(codes[adefs[i]]);
	list = set.x.ToVector();
	for (auto i : list)
		result.push_back(codes[xdefs[i]]);
	list = set.y.ToVector();
	for (auto i : list)
		result.push_back(codes[ydefs[i]]);
}
