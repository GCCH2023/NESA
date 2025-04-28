#pragma once
#include "CNodeVisitor.h"

struct Variable;

// 判断指定节点是否使用了指定变量，需要先执行遍历
// 目前的实现有点简单，没有考虑全局变量或字段等情况
class VariableUsed :
    public CNodeVisitor
{
public:
    VariableUsed(const Variable* variable_):
        variable(variable_),
        used(false)
    {
    }
    inline bool IsUsed() const { return used; }
protected:
    bool BeforeVisit(CNode* root) override;
    void OnVisit(Statement* node) override;
    void OnVisit(Expression* node) override;
private:
    const Variable* variable;
    bool used;
};

