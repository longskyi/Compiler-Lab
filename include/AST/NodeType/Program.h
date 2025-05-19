#ifndef M_AST_PROGRAM_TYPE_HEADER
#define M_AST_PROGRAM_TYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"

namespace AST
{
// Block -> { BlockItemList }  
// BlockItemList -> epsilon | BlockItemList BlockItem
// BlockItem -> Decl | Stmt
class Program;
class BlockItemList;
class BlockItem;
class Block;

class Program : public ASTNode
{
private:
    /* data */
public:
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Program -> DeclList"};
    Program(/* args */);
    ~Program();
};

Program::Program(/* args */)
{
}

Program::~Program()
{
}

class BlockItem : public ASTNode
{
private:
    /* data */
public:
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"BlockItem -> Decl",u8"BlockItem -> Stmt"};
    BlockItem(/* args */){}
    ~BlockItem(){}
};


class BlockItemList : public ASTNode
{
private:
    /* data */
public:
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"BlockItemList -> epsilon",u8"BlockItemList -> BlockItemList BlockItem"};
    BlockItemList(/* args */);
    ~BlockItemList();
};

BlockItemList::BlockItemList(/* args */)
{
}

BlockItemList::~BlockItemList()
{
}

class Block : public ASTNode
{
private:
    /* data */
public:
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Block -> { BlockItemList }"};
    Block(/* args */);
    ~Block();
};

Block::Block(/* args */)
{
}

Block::~Block()
{
}

} // namespace AST
#endif