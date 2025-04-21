#include "stdafx.h"
#include "TACFunction.h"
#include "CTranslator.h"
#include "Function.h"
#include "CDataBase.h"
#include "CASTTraverser.h"

CTranslator::CTranslator(Allocator& allocator_):
allocator(allocator_),
tacFunc(nullptr),
function(nullptr)
{
	for (int i = 0; i < 9; ++i)
	{
		registers[i] = GetCDB().AddString(ToString((TACRegister)i));
	}
}


CTranslator::~CTranslator()
{

}

Function* CTranslator::Translate(TACFunction* tacFunc)
{
	if (!tacFunc)
		return nullptr;
	Reset();
	this->tacFunc = tacFunc;

	Function* func = allocator.New<Function>();
	this->function = func;
	func->address = tacFunc->GetStartAddress();
	// 设置C函数的类型和参数
	SetFunctionType();
	// 创建临时变量
	SetLocalVariables();

	// 遍历控制树，生成C语句
	// COUT << root->statement;
	func->SetBody(TranslateBody());
	Sprintf<> s;
	s.Format(_T("sub_%04X"), GetTACFunction()->GetStartAddress());
	func->name = GetCDB().AddString(s.ToString());

	RemoveUnusedLocalVariables();

	return func;
}

void CTranslator::Reset()
{
	this->tacFunc = nullptr;
	this->function = nullptr;
	labels.clear();
	blockStatements.clear();
}

const Variable* CTranslator::GetLocalVariable(String* name, Type* type)
{
	auto v = this->function->GetVariable(name);
	if (v)
		return v;

	// 添加
	auto variable = allocator.New<Variable>();
	variable->name = name;
	variable->type = type;
	this->function->AddVariable(variable);
	return variable;
}

const Variable* CTranslator::GetLocalVariable(int index)
{
	// 三地址码中的临时变量和C函数的临时变量不是一一对应的
	// 必须根据名称来查找
	auto name = GetLocalVariableName(index);
	return function->GetVariable(name);
}

String* CTranslator::GetLocalVariableName(int index)
{
	auto& types = GetTACFunction()->GetTempVariableTypes();
	auto type = types[index];
	if (type == GetCDB().GetAXYType())
		return GetCDB().AddString(_T("axy"));

	Sprintf<> s;
	s.Format(_T("temp%d"), index);
	return GetCDB().AddString(s.ToString());
}


CNode* CTranslator::GetExpression(const TACOperand& operand)
{
	switch (operand.GetKind())
	{
	case TACOperand::INTEGER:
		return allocator.New<CNode>(operand.GetValue());
	case TACOperand::TEMP:
	{
							 auto var = GetLocalVariable(operand.GetValue());
							 return allocator.New<CNode>(var);
	}
	case TACOperand::REGISTER:
	{
								 // 寄存器要么是参数，要么是局部变量，不能当作全局变量处理
								 auto name = registers[operand.GetValue()];
								 auto variable = this->function->GetParameter(name);
								 if (variable)
									 return allocator.New<CNode>(variable);
								 variable = GetLocalVariable(name, TypeManager::Char);
								 return allocator.New<CNode>(variable);
	}
	case TACOperand::GLOBAL:
	{
							   uint32_t addr = operand.GetValue();
							   auto global = GetCDB().GetGlobalVariable(addr);
							   if (!global)
							   {
								   Sprintf<> s;
								   s.Format(_T("获取全局变量 %X 失败"), addr);
								   throw Exception(s.ToString());
							   }
							   return allocator.New<CNode>(global);
	}
	case TACOperand::ADDRESS:
	{
								uint32_t addr = operand.GetValue();
								auto global = GetCDB().GetGlobalVariable(addr);
								if (!global)
								{
									Sprintf<> s;
									s.Format(_T("获取全局变量 %X 失败"), addr);
									throw Exception(s.ToString());
								}
								return allocator.New<CNode>(global);
	}
	default:
	{
			   TCHAR buffer[64];
			   _stprintf_s(buffer, _T("三地址码转C语句：未实现的三地址码操作数转换"));
			   throw Exception(buffer);
	}
	}
}

CNode* CTranslator::GetExpression(CNode& node, const TACOperand& operand)
{
	switch (operand.GetKind())
	{
	case TACOperand::INTEGER:
		node = CNode(operand.GetValue());
		break;
	case TACOperand::TEMP:
	{
		auto var = GetLocalVariable(operand.GetValue());
		node = CNode(var);
		break;
	}
	case TACOperand::REGISTER:
	{
		// 寄存器要么是参数，要么是局部变量，不能当作全局变量处理
		auto name = registers[operand.GetValue()];
		auto variable = this->function->GetParameter(name);
		if (variable)
		{
			node = CNode(variable);
			break;
		}
		variable = GetLocalVariable(name, TypeManager::Char);
		node = CNode(variable);
		break;
	}
	case TACOperand::GLOBAL:
	{
		uint32_t addr = operand.GetValue();
		auto global = GetCDB().GetGlobalVariable(addr);
		if (!global)
		{
			Sprintf<> s;
			s.Format(_T("获取全局变量 %X 失败"), addr);
			throw Exception(s.ToString());
		}
		node = CNode(global);
		break;
	}
	case TACOperand::ADDRESS:
	{
		uint32_t addr = operand.GetValue();
		auto global = GetCDB().GetGlobalVariable(addr);
		if (!global)
		{
			Sprintf<> s;
			s.Format(_T("获取全局变量 %X 失败"), addr);
			throw Exception(s.ToString());
		}
		node = CNode(global);
		break;
	}
	default:
	{
		TCHAR buffer[64];
		_stprintf_s(buffer, _T("三地址码转C语句：未实现的三地址码操作数转换"));
		throw Exception(buffer);
	}
	}
	return &node;
}

void CTranslator::SetFunctionType()
{
	// 首先创建一个表示AXY寄存器的结构体
	Type* axyType = GetCDB().GetAXYType();

	// 创建函数类型
	Type funcType(TypeKind::Function);
	funcType.f.returnType = TypeManager::Void;
	if (this->GetTACFunction()->GetReturnFlag())
	{
		// 有返回值，那么就使用 AXY 结构体作为返回值
		funcType.f.returnType = axyType;
	}
	auto param = this->GetTACFunction()->GetParamFlag();
	Variable a;
	a.name = registers[Nes::NesRegisters::A];
	a.type = TypeManager::Char;
	TypeList aType = { TypeManager::Char, nullptr };
	Variable x;
	x.name = registers[Nes::NesRegisters::X];
	x.type = TypeManager::Char;
	TypeList xType = { TypeManager::Char, nullptr };
	Variable y;
	y.name = registers[Nes::NesRegisters::Y];
	y.type = TypeManager::Char;
	TypeList yType = { TypeManager::Char, nullptr };
	if (param)
	{
		if (param & (1 << Nes::NesRegisters::A))
		{
			funcType.AddParameter(&aType);
			this->function->AddParameter(allocator.New<Variable>(&a));
		}
		if (param & (1 << Nes::NesRegisters::X))
		{
			funcType.AddParameter(&xType);
			this->function->AddParameter(allocator.New<Variable>(&x));
		}
		if (param & (1 << Nes::NesRegisters::Y))
		{
			funcType.AddParameter(&yType);
			this->function->AddParameter(allocator.New<Variable>(&y));
		}
	}
	this->function->SetType(GetTypeManager().NewFunction(&funcType));
}


void CTranslator::SetLocalVariables()
{
	auto& types = this->GetTACFunction()->GetTempVariableTypes();
	int i = 0;
	for (auto type : types)
	{
		if (type == nullptr)
		{
			++i;
			continue;
		}
		// 添加
		auto variable = allocator.New<Variable>();
		variable->name = GetLocalVariableName(i);
		variable->type = type;
		this->function->AddVariable(variable);
		++i;
	}
}

CNode* CTranslator::TranslateBody()
{
	throw Exception(_T("翻译函数体未实现"));
}

String* CTranslator::GetLabelName(uint32_t jumpAddr)
{
	auto it = labels.find(jumpAddr);
	if (it == labels.end())
	{
		Sprintf<> s;
		s.Format(_T("L%04X"), jumpAddr);
		auto label = GetCDB().AddString(s.ToString());
		labels[jumpAddr] = label;
		return label;
	}
	return it->second;
}

void CTranslator::PatchLabels()
{
	for (auto label : labels)
	{
		auto statement = blockStatements[label.first];
		if (statement->kind != CNodeKind::STAT_LABEL)
		{
			// 修改为标签语句
			auto body = allocator.New<CNode>();
			*body = *statement;
			statement->kind = CNodeKind::STAT_LABEL;
			statement->l.body = body;
			statement->l.name = label.second;
		}
	}
}

void CTranslator::AddAddressMapStatement(uint32_t address, CNode* statement)
{
	blockStatements[address] = statement;
}

CNode* CTranslator::NewNoneStatement()
{
	return allocator.New<CNode>(CNodeKind::STAT_NONE);
}

CNode* CTranslator::NewStatementList(CNode* head, CNode* tail)
{
	if (!head)
		return NewNoneStatement();
	if (head == tail)
		return head;
	return allocator.New<CNode>(CNodeKind::STAT_LIST, head, tail);
}

class LocvalVariablesRemover : public CASTVisitor
{
public:
	LocvalVariablesRemover(Function* function_):function(function_){}
	void Run()
	{
		CASTTraverser traverser;
		traverser.Traverse(function->GetBody(), *this);

		std::erase_if(function->GetVariableList(), [&visited = visited](const Variable* var) {
			return visited.find(var) == visited.end();
			});
	}
protected:
	virtual void PreVisit(CNode* node)
	{
		if (node->kind == CNodeKind::EXPR_VARIABLE)
		{
			visited.insert(node->variable);
		}
	}
private:
	Function* function;
	std::unordered_set<const Variable*> visited;
};

void CTranslator::RemoveUnusedLocalVariables()
{
	LocvalVariablesRemover remover(GetFunction());
	remover.Run();
}
