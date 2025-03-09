#pragma once
#include "String.h"
#include "TypeManager.h"
#include "CNode.h"
#include "Function.h"

using GlobalList = std::vector<Variable*>;
using FunctionList = std::vector<Function*>;
using TagList = std::vector<Type*>;

// 管理C代码相关数据
class CDataBase
{
	friend CDataBase& GetCDB();
private:
	CDataBase(Allocator& allocator);
public:
	~CDataBase();

	// 添加指定字符串到字符串表中
	inline String* AddString(const TCHAR* str) { return stringTable.AddString(str); }
	// 获取指定字符串对象，不存在则返回空
	inline String* GetString(const TCHAR* str) { return stringTable.GetString(str); }

	// 获取全局变量列表
	inline const GlobalList& GetGlobalList() const { return globals; }
	// 根据地址获取全局变量，不存在返回 nullptr
	// 如果一个地址属于一个变量的范围内，则返回这个变量
	const Variable* GetGlobalVariable(CAddress address) const;
	// 根据名称获取全局变量，如果不存在，返回 nullptr
	const Variable* GetGlobalVariable(String* name) const;
	// 添加一个全局变量，失败抛出异常
	// 1. 这是用来描述NES代码的，所以地址和类型必须要有
	// 如果没有名称，则自动生成名称
	const Variable* AddGlobalVariable(CAddress address, Type* type, String* name = nullptr);
	// 修改包含指定地址的全局变量的类型
	// 如果新类型涵盖了其他全局变量，不会删除那些变量
	void SetGlobalVariableType(CAddress address, Type* type);
	// 删除指定地址范围 [start, end) 的全局变量
	void DeleteGlobalVariables(CAddress start, CAddress end);

	// 根据地址生成全局变量的名称
	String* MakeGlobalVariableName(CAddress address);

	// 获取函数列表
	inline const FunctionList& GetFunctionList() const { return functions; }
	// 添加一个函数
	void AddFunction(Function* function);

	// 获取标签列表
	inline const TagList& GetTagList() const { return tags; }
	// 添加一个标签
	void AddTag(Type* tag);
	// 根据名称获取标签类型
	Type* GetTag(String* name);

	// 获取分配器
	inline Allocator& GetAllocator() { return allocator; }

	// NES 专用返回值类型
	Type* GetAXYType();
protected:
	// 不做任何验证地添加全局变量
	void RawAddGlobalVariable(CAddress address, Type* type, String* name = nullptr, void* initializer = nullptr);
protected:
	Allocator& allocator;
	StringTable stringTable;

	GlobalList globals;  // 全局变量列表，按地址从小到大排列
	FunctionList functions;  // 函数列表
	TagList tags;  // 结构体，枚举，联合体列表

	Type* axyType;
};

// 获取C数据库单例
CDataBase& GetCDB();