#pragma once
#include "CBasicBlockBaseTranslator.h"

// 将三地址码基本块翻译为C代码
// 跳转指令不会被翻译C语句，而是保存为条件表达式和跳转目标地址
class CBasicBlockTranslator:
	public CBasicBlockBaseTranslator
{
public:
	CBasicBlockTranslator(CTranslator* translator);
};

