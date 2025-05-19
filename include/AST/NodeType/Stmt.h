#ifndef M_AST_STMT_TYPE_HEADER
#define M_AST_STMT_TYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"
#include"AST/NodeType/Expr.h"

namespace AST
{
    
//特别注意，需要支持Block
class Block;

class Stmt : public ASTNode 
{
    inline Stmt() {
        this->Ntype = ASTType::Stmt;
    }
};

class BlockStmt : public Stmt 
{
public :
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Stmt -> Block "};
};
class Assign : public Stmt 
{
public :
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Stmt -> id = Expr "};
};

class Branch : public Stmt 
{
public:
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Stmt -> if ( Bool ) Stmt",u8"Stmt -> if ( Bool ) Stmt else Stmt "};
};

class Loop : public Stmt 
{
public:
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Stmt -> while ( Bool ) Stmt "};
};

class FunctionCall : public Stmt 
{
public:
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"id ( ParamList ) ; "};
};

class Return : public Stmt 
{
public:
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Stmt -> return Expr ;"};
};


} // namespace AST


#endif