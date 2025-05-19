#ifndef M_AST_ARG_TYPE_HEADER
#define M_AST_ARG_TYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"


namespace AST
{
    
class ArgList : public ASTNode 
{
public:
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"ArgList -> epsilon",u8"ArgList -> Arg ; ArgList"};
};

class Arg : public ASTNode {};
    static constexpr std::array<std::u8string_view,3> SupportProd=
    {u8"Arg -> Type id",u8"Type id [ ]",u8"Type id ( )"};

} // namespace AST


#endif