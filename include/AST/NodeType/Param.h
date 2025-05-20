#ifndef M_AST_PARAM_HEADER
#define M_AST_PARAM_HEADER
#include "AST/NodeType/ASTbaseType.h"
//#include "AST/NodeType/Expr.h"
#include "AST/NodeType/NodeBase.h"
#include "lcmpfileio.h"
#include <numeric>

namespace AST
{
    
class Expr;

enum ParamOp {
    None,
    expr,
    ARRAY_PTRP,
    FUNC_PTRP,
};

class Param : public ASTNode 
{
public:
    ParamOp op = ParamOp::None;
    unique_ptr<Expr> expr_ptr;
    unique_ptr<SymIdNode> id_ptr;
    static constexpr std::array<std::u8string_view,3> SupportProd=
    {u8"Param -> Expr",u8"Param -> id [ ]",u8"Param -> id ( )"};
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Param::try_constructS(as,astTree);
    }
    Param() {
        this->Ntype = ASTType::Param;
        this->subType = ASTSubType::Param;
    }
    void accept(ASTVisitor & visitor) override;
};

class ParamList : public ASTNode 
{
public:
    std::vector<unique_ptr<Param>> paramList;
    static constexpr std::array<std::u8string_view,3> SupportProd=
    {u8"ParamList -> epsilon",u8"ParamList -> Param , ParamList",u8"ParamList -> Param"};
    inline static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
        auto * NonTnode = dynamic_cast<NonTermProdNode *>(as);
        if(!NonTnode) {
            std::cerr << "Compiler internal Error ParamList 不接受非NonTermProdNode的try contrust请求";
            return nullptr;
            //throw std::runtime_error("ERROR,ConstExpr不接受非NonTermProdNode的try contrust请求");
        }
        //检查 str prod匹配
        int targetProd = 0;
        for(targetProd = 0 ; targetProd < SupportProd.size() ; targetProd++) {
            U8StrProduction u8prod = LCMPFileIO::parseProduction(SupportProd[targetProd]);
            if(equals(u8prod,astTree->Productions.at(NonTnode->prodId),astTree->symtab)) {
                break;
            }
        }
        if(targetProd == SupportProd.size()) {
            return nullptr;
        }
        switch (targetProd)
        {
        case 0:
        {
            auto newNode = std::make_unique<ParamList>();
            return newNode;
        }
        case 1:
        {
            //ParamList -> Param , ParamList
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<ParamList *>(NonTnode->childs[2].get()) && "不是List类型节点");
            assert(dynamic_cast<Param *>(NonTnode->childs[0].get()) && "不是Item类型节点");
            unique_ptr<ParamList> addNode;
            addNode.reset(static_cast<ParamList*>(NonTnode->childs[2].release()));
            unique_ptr<Param> frontNode;
            frontNode.reset(static_cast<Param *>(NonTnode->childs[0].release()));
            addNode->paramList.emplace(addNode->paramList.begin(), std::move(frontNode));
            return addNode;
        }
        case 2:
        {
            //ParamList -> Param
            assert(NonTnode->childs.size() == 1);
            assert(dynamic_cast<Param *>(NonTnode->childs[0].get()) && "是List类型节点");
            auto newNode = std::make_unique<ParamList>();
            unique_ptr<Param> addNode;
            addNode.reset(static_cast<Param*>(NonTnode->childs[0].release()));
            newNode->paramList.emplace_back(std::move(addNode));
            return newNode;
        }
        default:
            std::cerr <<"Not implement BlockItemList Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return ParamList::try_constructS(as,astTree);
    }
    ParamList() {
        this->Ntype = ASTType::ParamList;
        this->subType = ASTSubType::ParamList;
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        for(const auto & p : paramList) {
            p->accept(visitor);
        }
        visitor.quit(this);
        return;
    }
};

} // namespace AST


#endif