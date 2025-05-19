#ifndef LCMP_AST_HEADER
#define LCMP_AST_HEADER
#include <memory>
#include <string>
#include <stack>
#include <stdexcept>
#include <fstream>
#include "templateUtil.h"
#include "SyntaxType.h"
#include "lexer.h"
#include"parserGen.h"
#include"stringUtil.h"
#include"fileIO.h"
#include"AST/NodeType/ASTbaseType.h"
namespace AST
{


extern StateId startStateId;


void printCommonAST(const unique_ptr<ASTNode>& node, int depth = 0);

void AST_test_main();

} // namespace AST


#endif