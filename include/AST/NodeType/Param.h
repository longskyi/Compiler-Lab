#ifndef M_AST_ARG_TYPE_HEADER
#define M_AST_ARG_TYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"


namespace AST
{
    
class ParamList : public ASTNode 
{
public:
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"ParamList -> epsilon",u8"ParamList -> Param , ParamList"};
};

class Param : public ASTNode 
{
public:
    static constexpr std::array<std::u8string_view,3> SupportProd=
    {u8"Param -> Expr",u8"Param -> id [ ]",u8"Param -> id ( )"};
};


} // namespace AST


#endif