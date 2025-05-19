#ifndef M_AST_DECLARE_TYPE_HEADER
#define M_AST_DECLARE_TYPE_HEADER
#include<optional>
#include"AST/NodeType/ASTbaseType.h"
#include"AST/NodeType/Type.h"
#include"AST/NodeType/Expr.h"

namespace AST
{
    
class Declare : public ASTNode {};

class IdDeclare : public Declare 
{
public:
    unique_ptr<pType> type_ptr;
    unique_ptr<SymIdNode> id_ptr;
    std::optional<int> array_len;
    std::optional<Expr> initExpr;
    static constexpr std::array<std::u8string_view,3> SupportProd=
    {u8"Decl -> Type id ;",u8"Decl -> Type id = Expr ; " u8"Decl -> Type id [ num ] ;" };
};

class FuncDeclare : public Declare 
{
public:
    unique_ptr<pType> type_ptr;
    unique_ptr<SymIdNode> id_ptr;
    unique_ptr<ASTNode> ArgList_ptr;
    unique_ptr<ASTNode> Block_ptr;
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Decl -> Type id ( ArgList ) Block"};
};



} // namespace AST


#endif