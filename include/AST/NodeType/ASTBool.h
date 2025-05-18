#ifndef M_AST_BOOL_TYPE_HEADER
#define M_AST_BOOL_TYPE_HEADER
#include "AST/NodeType/ASTbaseType.h"

namespace AST
{


class ASTBool : public ASTNode
{
private:
    /* data */
public:
    ASTBool(/* args */);
    ~ASTBool();
};

ASTBool::ASTBool(/* args */)
{
}

ASTBool::~ASTBool()
{
}


} // namespace AST


#endif