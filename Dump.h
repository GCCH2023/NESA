#pragma once
#include "CNode.h"
#include "Type.h"
#include "Function.h"

struct String;

// 指定缩进输出抽象语法树表示的C语句
OStream& DumpCNode(OStream& os, const CNode* root, int indent);
// 输出抽象语法树表示的C语句
OStream& operator<<(OStream& os, const CNode* root);
// 输出抽象语法树的语句结构（不输出表达式）
OStream& DumpCNodeStructures(OStream& os, const CNode* root, int indent);
OStream& operator<<(OStream& os, const String* str);


// 输出结构体或联合体或函数的定义
void DumpDefinition(Type* type);
void DumpDefinition(Function* func);


// 输出函数声明
void DumpDeclaration(Function* func);
