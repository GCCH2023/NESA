#pragma once
class Allocator;

// ���ڽ�AST�޸�ΪDAG
class ASTManager
{
public:
	ASTManager(Allocator& allocator);
	~ASTManager();

protected:
	Allocator& allocator;
};

