#ifndef M_AST_STMT_TYPE_HEADER
#define M_AST_STMT_TYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"


namespace AST
{
    
//特别注意，需要支持Block


class Stmt : public ASTNode {};

class Assign : public Stmt {};

class Branch : public Stmt {};

class Loop : public Stmt {};

class FuncionCall : public Stmt {};

class Return : public Stmt {};


} // namespace AST


#endif