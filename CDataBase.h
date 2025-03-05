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
public:
	CDataBase(Allocator& allocator);
	~CDataBase();

	// 添加指定字符串到字符串表中
	inline String* AddString(const TCHAR* str) { return stringTable.AddString(str); }
	// 获取指定字符串对象，不存在则返回空
	inline String* GetString(const TCHAR* str) { return stringTable.GetString(str); }

	TypeManager& GetTypeManager() { return typeManager; }

	// 获取全局变量列表
	inline const GlobalList& GetGlobalList() const { return globals; }
	// 根据地址获取全局变量，不存在返回 nullptr
	const Variable* GetGlobalVariable(CAddress address) const;
	// 根据名称获取全局变量，如果不存在，返回 nullptr
	const Variable* GetGlobalVariable(String* name) const;
	// 添加一个全局变量，失败抛出异常
	const Variable* AddGlobalVariable(String* name, Type* type, CAddress address);


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
protected:
	// 不做任何验证地添加全局变量
	void RawAddGlobalVariable(String* name, Type* type, CAddress address, void* initializer = nullptr);
protected:
	Allocator& allocator;
	StringTable stringTable;
	TypeManager typeManager;

	GlobalList globals;  // 全局变量列表，按地址从小到大排列
	FunctionList functions;  // 函数列表
	TagList tags;  // 结构体，枚举，联合体列表
};

