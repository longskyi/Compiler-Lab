#ifndef M_AST_ARG_TYPE_HEADER
#define M_AST_ARG_TYPE_HEADER
#include"lcmpfileio.h"
#include"AST/NodeType/ASTbaseType.h"


namespace AST
{
//pType只在构建过程中出现，最终会被折叠进声明或者Arg
class pType : public ASTNode 
{
public:
    SymType type;
    static constexpr std::array<std::u8string_view,5> SupportProd=
    {u8"Type -> BaseType", u8"Type -> BaseType *",  
    u8"BaseType -> int",u8"BaseType -> float",u8"BaseType -> void "};
    pType() {
        this->Ntype = ASTType::pType;
        this->subType = ASTSubType::pType;
    }
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
            assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
            return std::move(NonTnode->childs[0]);
            break;
        case 1:
        {
            std::unique_ptr<pType> newNode(
                static_cast<pType*>(NonTnode->childs[0].release())
            );
            // if(newNode->type.eType != baseType::NonInit) {
            //     std::unreachable();
            // }
            newNode->type.makePtr();
            return newNode;
        }
        case 2:
        {
            //int
            unique_ptr <pType> newNode = std::make_unique<pType>();
            newNode->type.basicType = baseType::INT;
            return newNode;
        }
        case 3:
        {
            //float
            unique_ptr <pType> newNode = std::make_unique<pType>();
            newNode->type.basicType = baseType::FLOAT;
            return newNode;
        }
        case 4:
        {
            //void
            unique_ptr <pType> newNode = std::make_unique<pType>();
            newNode->type.basicType = baseType::VOID;
            return newNode;
        }
        default:
            std::cerr <<"Not implement pTypen Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return pType::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        visitor.quit(this);
        return;
    }
};

//Not implement
class pTypeList : public ASTNode {

};

} // namespace AST


#endif