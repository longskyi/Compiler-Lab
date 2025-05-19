#ifndef M_AST_BOOL_TYPE_HEADER
#define M_AST_BOOL_TYPE_HEADER
#include "AST/NodeType/ASTbaseType.h"

namespace AST
{


class ASTBool : public ASTNode
{
public:
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Bool -> Expr rop Expr",u8"Bool -> Expr" };
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