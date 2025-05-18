#ifndef MAST_BASE_TYPE_HEADER
#define MAST_BASE_TYPE_HEADER
#include<memory>
#include<string>
#include<SyntaxType.h>
#include<stringUtil.h>
namespace AST {


using u8string = std::u8string;

using std::unique_ptr;


enum class ASTType {
    None,
    Common,
    Expr,
};

enum class ASTSubType {
    None,
    SubCommon,

    Expr,
    ConstExpr,
    DeferExpr,
    CallExpr,
    ArithExpr,

};

class ASTVisitor;

class ASTNode
{
public:
    ASTType Ntype;
    ASTSubType subType;
    virtual void accept(ASTVisitor& visitor) = 0; 
    inline ASTNode() {
        Ntype = ASTType::None;
        subType = ASTSubType::None;
    } 
    virtual ~ASTNode() = default;
};


class ASTVisitor {
public:
    virtual void visit(ASTNode* node) = 0;
};


class ASTCommonNode : public ASTNode
{
public:
    inline ASTCommonNode() {
        Ntype = ASTType::Common;
        subType = ASTSubType::SubCommon;
    }
    inline bool isLeaf() {
        return childs.size() == 0;
    }
    inline void accept(ASTVisitor & visitor) override {
        //前序遍历
        visitor.visit(this);
        for(auto & kid : childs) {
            kid->accept(visitor);
        }
        return;
    }
    u8string type; //原始类型-非终结符
    u8string value;
    std::vector<unique_ptr<ASTNode>> childs;
};

class mVisitor : public ASTVisitor {
    inline void visit(ASTNode * node) override {
        if(dynamic_cast<ASTCommonNode*>(node)) {
            this->visit(static_cast<ASTCommonNode*>(node));
        }
        return;
    }
    inline void visit(ASTCommonNode * node) {
        std::cerr << toString(node->value)<<" ";
        return;
    }
};


}

#endif