#ifndef M_AST_DECLARE_TYPE_HEADER
#define M_AST_DECLARE_TYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"


namespace AST
{
    
class Declare : public ASTNode {};

class IdDeclare : public Declare {};

class FuncDeclare : public Declare {};



} // namespace AST


#endif