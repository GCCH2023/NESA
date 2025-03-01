#pragma once
#include "String.h"
#include "TypeManager.h"
#include "CNode.h"
#include "Function.h"

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
protected:
	Allocator& allocator;
	StringTable stringTable;
	TypeManager typeManager;
};

