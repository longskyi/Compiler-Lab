#include"AST/NodeType/ASTbaseType.h"

namespace AST 
{
const char* ASTTypeToString(ASTType type) {
    switch (type) {
        case ASTType::None: return "None";
        case ASTType::Common: return "Common";
        case ASTType::TermSym: return "TermSym";
        case ASTType::NonTermPordNode: return "NonTermPordNode";
        case ASTType::SymIdNode : return "SymIdNode";
        case ASTType::Expr: return "Expr";
        case ASTType::ArgList: return "ArgList";
        case ASTType::Arg: return "Arg";
        case ASTType::ASTBool: return "ASTBool";
        case ASTType::Declare: return "Declare";
        case ASTType::ParamList: return "ParamList";
        case ASTType::Param: return "Param";
        case ASTType::Program: return "Program";
        case ASTType::BlockItemList: return "BlockItemList";
        case ASTType::BlockItem: return "BlockItem";
        case ASTType::Block: return "Block";
        case ASTType::Stmt: return "Stmt";
        case ASTType::pType: return "pType";
        default: return "Unknown ASTType";
    }
}

const char* ASTSubTypeToString(ASTSubType subtype) {
    switch (subtype) {
        case ASTSubType::None: return "None";
        case ASTSubType::SubCommon: return "SubCommon";
        case ASTSubType::TermSym: return "TermSym";
        case ASTSubType::NonTermPordNode: return "NonTermPordNode";
        case ASTSubType::SymIdNode : return "SymIdNode";
        case ASTSubType::Expr: return "Expr";
        case ASTSubType::ConstExpr: return "ConstExpr";
        case ASTSubType::DerefExpr: return "DerefExpr";
        case ASTSubType::CallExpr: return "CallExpr";
        case ASTSubType::ArithExpr: return "ArithExpr";
        case ASTSubType::RightValueExpr: return "RightValueExpr";
        case ASTSubType::ArgList: return "ArgList";
        case ASTSubType::Arg: return "Arg";
        case ASTSubType::ASTBool: return "ASTBool";
        case ASTSubType::Declare: return "Declare";
        case ASTSubType::IdDeclare: return "IdDeclare";
        case ASTSubType::FuncDeclare: return "FuncDeclare";
        case ASTSubType::ParamList: return "ParamList";
        case ASTSubType::Param: return "Param";
        case ASTSubType::Program: return "Program";
        case ASTSubType::BlockItemList: return "BlockItemList";
        case ASTSubType::BlockItem: return "BlockItem";
        case ASTSubType::Block: return "Block";
        case ASTSubType::Stmt: return "Stmt";
        case ASTSubType::BlockStmt: return "BlockStmt";
        case ASTSubType::Assign: return "Assign";
        case ASTSubType::Branch: return "Branch";
        case ASTSubType::Loop: return "Loop";
        case ASTSubType::FunctionCall: return "FunctionCall";
        case ASTSubType::Return: return "Return";
        case ASTSubType::pType: return "pType";
        default: return "Unknown ASTSubType";
    }
}

}

