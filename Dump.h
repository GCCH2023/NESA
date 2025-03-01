#pragma once
#include "CNode.h"
#include "Type.h"
#include "Function.h"

struct String;

// ָ��������������﷨����ʾ��C���
OStream& DumpCNode(OStream& os, const CNode* root, int indent);
// ��������﷨����ʾ��C���
OStream& operator<<(OStream& os, const CNode* root);
// ��������﷨�������ṹ����������ʽ��
OStream& DumpCNodeStructures(OStream& os, const CNode* root, int indent);
OStream& operator<<(OStream& os, const String* str);


// ����ṹ�������������Ķ���
void DumpDefinition(Type* type);
void DumpDefinition(Function* func);


// �����������
void DumpDeclaration(Function* func);
