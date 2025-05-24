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
    EQ,
    NoneROP,
};
enum BOOLOP_ENUM {
    BOOLAND,
    BOOLOR,
    BOOLNone
};

class ASTBool : public ASTNode
{
public:
    bool isBoolOperation = false;
    unique_ptr<Expr> Lval_ptr;
    ROP_ENUM rop = NoneROP;
    BOOLOP_ENUM BoolOP = BOOLNone;
    unique_ptr<Expr> Rval_ptr;
    unique_ptr<ASTBool> LBool;
    unique_ptr<ASTBool> RBool;
    static constexpr std::array<std::u8string_view,5> SupportProd=
    {u8"Bool -> Expr rop Expr",u8"Bool -> Expr",u8"Bool -> Bool and Bool",u8"Bool -> Bool or Bool" ,
        u8"Bool -> ( Bool )"};
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
            } else if(rop_ptr->value == u8"==") {
            newNode->rop = ROP_ENUM::EQ;
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
        case 2:
        {
            // bool and bool
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<ASTBool *>(NonTnode->childs[0].get()) && "不是Bool类型节点");

            auto newNode = std::make_unique<ASTBool>();
            newNode->isBoolOperation = true;
            newNode->LBool.reset(static_cast<ASTBool *>(NonTnode->childs[0].release()));
            newNode->RBool.reset(static_cast<ASTBool *>(NonTnode->childs[2].release()));
            
            newNode->BoolOP = BOOLAND;

            return newNode;
        }
        case 3:
        {
            // bool or bool
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<ASTBool *>(NonTnode->childs[0].get()) && "不是Bool类型节点");

            auto newNode = std::make_unique<ASTBool>();
            newNode->isBoolOperation = true;
            newNode->LBool.reset(static_cast<ASTBool *>(NonTnode->childs[0].release()));
            newNode->RBool.reset(static_cast<ASTBool *>(NonTnode->childs[2].release()));
            
            newNode->BoolOP = BOOLOR;

            return newNode;
        }
        case 4:
        {
            // ( bool )
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<ASTBool *>(NonTnode->childs[1].get()) && "不是Bool类型节点");

            unique_ptr<ASTBool> stoleNode = nullptr;
            
            stoleNode.reset(static_cast<ASTBool *>(NonTnode->childs[1].release()));
            
            return stoleNode;
        }
        default:
            std::cerr <<"Not implement ASTBool Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return ASTBool::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        if(!isBoolOperation) {
            Lval_ptr->accept(visitor);
            if(this->rop != ROP_ENUM::NoneROP) {
                Rval_ptr->accept(visitor);
            }
        }
        else {
            LBool->accept(visitor);
            RBool->accept(visitor);
        }
        visitor.visit(this);
        visitor.quit(this);
    }
};



} // namespace AST


#endif