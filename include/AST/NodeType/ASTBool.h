#ifndef M_AST_BOOL_TYPE_HEADER
#define M_AST_BOOL_TYPE_HEADER
#include "AST/NodeType/ASTbaseType.h"
#include "AST/NodeType/Expr.h"
namespace AST
{

enum ROP_ENUM {
    //GE, //greater equal
    LE, //lower equal
    L,  //lower
    G,  //greater
    NoneROP,
};

class ASTBool : public ASTNode
{
public:
    unique_ptr<Expr> Lval_ptr;
    ROP_ENUM rop = NoneROP;
    unique_ptr<Expr> Rval_ptr;
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Bool -> Expr rop Expr",u8"Bool -> Expr" };
    inline ASTBool(/* args */) {
        this->Ntype = ASTType::ASTBool;
        this->subType = ASTSubType::ASTBool;
    }
    inline ~ASTBool(){}
    inline static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
        auto * NonTnode = dynamic_cast<NonTermProdNode *>(as);
        if(!NonTnode) {
            std::cerr << "Compiler internal Error ptype 不接受非NonTermProdNode的try contrust请求";
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
            // Expr rop Expr
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<Expr *>(NonTnode->childs[0].get()) && "不是Expr类型节点");
            assert(dynamic_cast<Expr *>(NonTnode->childs[2].get()) && "不是Expr类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ROP");

            auto newNode = std::make_unique<ASTBool>();
            newNode->Lval_ptr.reset(static_cast<Expr *>(NonTnode->childs[0].release()));
            newNode->Rval_ptr.reset(static_cast<Expr *>(NonTnode->childs[2].release()));
            auto * rop_ptr = static_cast<TermSymNode*>(NonTnode->childs[1].get());
            if(rop_ptr->value == u8"<") {
                newNode->rop = ROP_ENUM::L;
            } else if(rop_ptr->value == u8"<=") {
                newNode->rop = ROP_ENUM::LE;
            } else if(rop_ptr->value == u8">") {
                newNode->rop = ROP_ENUM::G;
            } else {
                std::cerr<<"ROP not implement"<<toString_view(rop_ptr->value)<<std::endl;
                return nullptr;
            }
            return newNode;
        }
        case 1:
        {
            // Expr
            assert(NonTnode->childs.size() == 1);
            assert(dynamic_cast<Expr *>(NonTnode->childs[0].get()) && "不是Expr类型节点");

            auto newNode = std::make_unique<ASTBool>();
            newNode->Lval_ptr.reset(static_cast<Expr *>(NonTnode->childs[0].release()));
            
            newNode->rop = ROP_ENUM::NoneROP;

            return newNode;
        }
        default:
            std::cerr <<"Not implement ArithExpr Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return ASTBool::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        Lval_ptr->accept(visitor);
        if(this->rop != ROP_ENUM::NoneROP) {
            Rval_ptr->accept(visitor);
        }
        visitor.visit(this);
        visitor.quit(this);
    }
};



} // namespace AST


#endif