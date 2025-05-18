#ifndef M_AST_PROGRAM_TYPE_HEADER
#define M_AST_PROGRAM_TYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"

namespace AST
{
    
class Program : public ASTNode
{
private:
    /* data */
public:
    Program(/* args */);
    ~Program();
};

Program::Program(/* args */)
{
}

Program::~Program()
{
}

class BlockItemList : public ASTNode
{
private:
    /* data */
public:
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