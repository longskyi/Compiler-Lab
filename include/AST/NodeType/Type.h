#ifndef M_AST_ARG_TYPE_HEADER
#define M_AST_ARG_TYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"


namespace AST
{
    
class pType : public ASTNode 
{
public:
    SymType type;
    static constexpr std::array<std::u8string_view,5> SupportProd=
    {u8"Type -> BaseType", u8"Type -> BaseType *",  
     u8"BaseType -> int",u8"BaseType -> float",u8"BaseType -> void "};
};


} // namespace AST


#endif