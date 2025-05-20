#ifndef M_AST_PROGRAM_TYPE_HEADER
#define M_AST_PROGRAM_TYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"
#include "AST/NodeType/NodeBase.h"
#include<vector>
namespace AST
{
// Block -> { BlockItemList }  
// BlockItemList -> epsilon | BlockItemList BlockItem
// BlockItem -> Decl | Stmt
class Program;
class BlockItemList;
class BlockItem;
class Block;



//Block Item应当只在构建时临时出现，构建后不存在
class BlockItem : public ASTNode
{
public:
    unique_ptr<ASTNode> Item;
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"BlockItem -> Decl",u8"BlockItem -> Stmt"};
    BlockItem(/* args */){
        this->Ntype = ASTType::BlockItem;
        this->subType = ASTSubType::BlockItem;
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
        case 1:
        {
            //
            assert(NonTnode->childs.size() == 1);
            if(targetProd == 0) {
                assert(dynamic_cast<Declare *>(NonTnode->childs[0].get()));
            }
            else {
                assert(dynamic_cast<Stmt *>(NonTnode->childs[0].get())); 
            }
            auto  newNode = std::make_unique<BlockItem>();
            newNode->Item.reset(NonTnode->childs[0].release());
            return newNode;
        }
        default:
            std::cerr <<"Not implement BlockItem Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return BlockItem::try_constructS(as,astTree);
    }
    ~BlockItem(){}
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        this->Item->accept(visitor);
        visitor.quit(this);
        return;
    }
};


class BlockItemList : public ASTNode
{
public:
    std::vector<unique_ptr<ASTNode>> ItemList;  //Decl * Stmt *
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"BlockItemList -> epsilon",u8" BlockItemList -> BlockItemList BlockItem"};
    BlockItemList(/* args */){
        this->Ntype = ASTType::BlockItemList;
        this->subType = ASTSubType::BlockItemList;
    }
    ~BlockItemList(){}
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
            auto newNode = std::make_unique<BlockItemList>();
            return newNode;
        }
        case 1:
        {
            assert(NonTnode->childs.size() == 2);
            assert(dynamic_cast<BlockItemList *>(NonTnode->childs[0].get()) && "是List类型节点");
            assert(dynamic_cast<BlockItem *>(NonTnode->childs[1].get()) && "是Item类型节点");
            unique_ptr<BlockItemList> addNode;
            addNode.reset(static_cast<BlockItemList*>(NonTnode->childs[0].release()));
            addNode->ItemList.emplace_back(std::move(static_cast<BlockItem *>(NonTnode->childs[1].get())->Item));
            return addNode;
        }
        default:
            std::cerr <<"Not implement BlockItemList Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return BlockItemList::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        for(const auto & item : ItemList) {
            item->accept(visitor);
        }
        visitor.quit(this);
        return;
    }
};

class Block : public ASTNode
{
private:
    /* data */
public:
    unique_ptr<BlockItemList> ItemList_ptr;
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Block -> { BlockItemList }"};
    Block(/* args */);
    ~Block();
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
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<BlockItemList *>(NonTnode->childs[1].get()) && "是List类型节点");
            unique_ptr<Block> newNode = std::make_unique<Block>();
            newNode->ItemList_ptr.reset(static_cast<BlockItemList*>(NonTnode->childs[1].release()));
            return newNode;
        }
        default:
            std::cerr <<"Not implement Block Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Block::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        this->ItemList_ptr->accept(visitor);
        visitor.quit(this);
        return;
    }
};

inline Block::Block(/* args */)
{
    this->Ntype = ASTType::Block;
    this->subType = ASTSubType::Block;
}

inline Block::~Block()
{
}


class Program : public ASTNode
{
private:
    /* data */
public:
    unique_ptr<BlockItemList> ItemList;
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Program -> BlockItemList"};
    Program(/* args */);
    ~Program();
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Program::try_constructS(as,astTree);
    }
    void accept(ASTVisitor& visitor) override;
};

inline Program::Program(/* args */)
{
    this->Ntype = ASTType::Program;
    this->subType = ASTSubType::Program;
}

inline Program::~Program()
{
}

} // namespace AST
#endif