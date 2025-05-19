#ifndef M_AST_ARG_HEADER
#define M_AST_ARG_HEADER
#include"AST/NodeType/ASTbaseType.h"
#include<memory>
#include<vector>

namespace AST
{
    

class Arg : public ASTNode 
{
public:
    SymType argtype;
    unique_ptr<SymIdNode> id_ptr;
    static constexpr std::array<std::u8string_view,3> SupportProd=
    {u8"Arg -> Type id",u8"Arg -> Type id [ ]",u8"Arg -> Type id ( )"};
    inline Arg() {
        this->Ntype = ASTType::Arg;
        this->subType = ASTSubType::Arg;
    } 
    inline ~Arg() {}
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
            //Type id
            assert(NonTnode->childs.size() == 2);
            assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ID");

            auto newNode = std::make_unique<Arg>();
            newNode->argtype = (static_cast<pType*>(NonTnode->childs[0].get()))->type;
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            return newNode;
        }
        case 1:
        {
            //Type id [ ] 
            assert(NonTnode->childs.size() == 6);
            assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ID");
            auto newNode = std::make_unique<Arg>();
            newNode->argtype = (static_cast<pType*>(NonTnode->childs[0].get()))->type;
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            newNode->argtype.eeType = newNode->argtype.eType;
            newNode->argtype.eType = newNode->argtype.Type;
            newNode->argtype.Type = baseType::ARRAY_PTR;
            return newNode;
        }
        case 2:
        {
            //Type id ( )
            assert(NonTnode->childs.size() == 6);
            assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ID");
            auto newNode = std::make_unique<Arg>();
            newNode->argtype = (static_cast<pType*>(NonTnode->childs[0].get()))->type;
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            newNode->argtype.eeType = newNode->argtype.eType;
            newNode->argtype.eType = newNode->argtype.Type;
            newNode->argtype.Type = baseType::FUNC_PTR;
            return newNode;
        }
        default:
            std::cerr <<"Not implement Arg Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Arg::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        this->id_ptr->accept(visitor);
        visitor.visit(this);
        visitor.quit(this);
        return;
    }
};

class ArgList : public ASTNode 
{
public:
    std::vector<unique_ptr<Arg>> argLists;
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"ArgList -> epsilon",u8"ArgList -> Arg ; ArgList"};
    inline ArgList() {
        this->Ntype = ASTType::ArgList;
        this->subType = ASTSubType::ArgList;
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
        {
            auto newNode = std::make_unique<ArgList>();
            return newNode;
        }
        case 1:
        {
            assert(NonTnode->childs.size() == 2);
            assert(dynamic_cast<ArgList *>(NonTnode->childs[0].get()) && "是List类型节点");
            assert(dynamic_cast<Arg *>(NonTnode->childs[1].get()) && "是Item类型节点");
            unique_ptr<ArgList> addNode;
            addNode.reset(static_cast<ArgList*>(NonTnode->childs[0].release()));
            addNode->argLists.emplace_back(std::move(static_cast<Arg *>(NonTnode->childs[1].release())));
            return addNode;
        }
        default:
            std::cerr <<"Not implement ArgList Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return ArgList::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        for(const auto & item : argLists) {
            item->accept(visitor);
        }
        visitor.quit(this);
        return;
    }
};

} // namespace AST


#endif