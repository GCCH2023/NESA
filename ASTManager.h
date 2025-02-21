#pragma once
class Allocator;

// 用于将AST修改为DAG
class ASTManager
{
public:
	ASTManager(Allocator& allocator);
	~ASTManager();

protected:
	Allocator& allocator;
};

