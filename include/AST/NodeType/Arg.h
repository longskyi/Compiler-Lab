#ifndef M_AST_ARG_HEADER
#define M_AST_ARG_HEADER
#include"AST/NodeType/ASTbaseType.h"
#include "AST/NodeType/Type.h"
#include "AST/NodeType/NodeBase.h"
#include "lcmpfileio.h"
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
    {u8"Arg -> Type id",u8"Arg -> Type id [ ] Dimensions",u8"Arg -> Type id ( TypeList )"};
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
            newNode->argtype = std::move((static_cast<pType*>(NonTnode->childs[0].get()))->type);
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            return newNode;
        }
        case 1:
        {
            //Type id [ ] Dimensions
            assert(NonTnode->childs.size() == 5);
            assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ID");
            assert(dynamic_cast<Dimensions *>(NonTnode->childs[4].get()) && "是基本类型节点");
            auto newNode = std::make_unique<Arg>();
            newNode->argtype = std::move((static_cast<pType*>(NonTnode->childs[0].get()))->type);
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            const auto& dims = static_cast<Dimensions*>(NonTnode->childs[4].get())->array_len_vec;
            for (int i = dims.size() - 1; i >= 0; --i) {
                newNode->argtype.makeArray(dims[i]);
            }
            newNode->argtype.makePtr();
            return newNode;
        }
        case 2:
        {
            //Type id ( TypeList )
            assert(NonTnode->childs.size() == 5);
            assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
            assert(dynamic_cast<pTypeList *>(NonTnode->childs[3].get()) && "是list类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ID");
            auto newNode = std::make_unique<Arg>();
            newNode->argtype = std::move((static_cast<pType*>(NonTnode->childs[0].get()))->type);
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            auto VecType = std::vector<SymType>();
            auto listnode_ptr = unique_ptr<pTypeList>();
            listnode_ptr.reset(static_cast<pTypeList *>(NonTnode->childs[3].release()));
            for(const auto & typei : listnode_ptr->typeLists) {
                VecType.push_back(typei->type);
            }
            newNode->argtype.makeFuncPtr(VecType);
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
    static constexpr std::array<std::u8string_view,3> SupportProd=
    //ArgList -> epsilon | Arg ; ArgList | Arg
    {u8"ArgList -> epsilon",u8"ArgList -> Arg ; ArgList",u8"ArgList -> Arg"};
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
            //ArgList -> Arg ; ArgList
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<ArgList *>(NonTnode->childs[2].get()) && "不是List类型节点");
            assert(dynamic_cast<Arg *>(NonTnode->childs[0].get()) && "不是Item类型节点");
            unique_ptr<ArgList> addNode;
            addNode.reset(static_cast<ArgList*>(NonTnode->childs[2].release()));
            unique_ptr<Arg> frontNode;
            frontNode.reset(static_cast<Arg *>(NonTnode->childs[0].release()));
            addNode->argLists.emplace(addNode->argLists.begin(), std::move(frontNode));
            return addNode;
        }
        case 2:
        {
            //ArgList -> Arg
            assert(NonTnode->childs.size() == 1);
            assert(dynamic_cast<Arg *>(NonTnode->childs[0].get()) && "是List类型节点");
            auto newNode = std::make_unique<ArgList>();
            unique_ptr<Arg> addNode;
            addNode.reset(static_cast<Arg*>(NonTnode->childs[0].release()));
            newNode->argLists.emplace_back(std::move(addNode));
            return newNode;
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