#ifndef LCMP_AST_HEADER
#define LCMP_AST_HEADER
#include <cstdint>
#include <memory>
#include <string>
#include <stack>
#include <stdexcept>
#include <fstream>
#include <variant>
#include <stack>
#include "templateUtil.h"
#include "SyntaxType.h"
#include "lexer.h"
#include"parserGen.h"
#include"stringUtil.h"
#include"lcmpfileio.h"

#include"AST/NodeType/ASTbaseType.h"
#include"AST/NodeType/Expr.h"
#include"AST/NodeType/Program.h"
#include"AST/NodeType/Arg.h"
#include"AST/NodeType/ASTBool.h"
#include"AST/NodeType/Declare.h"
#include"AST/NodeType/Param.h"
#include"AST/NodeType/Stmt.h"
#include"AST/NodeType/Type.h"

namespace AST
{



extern StateId startStateId;


void printCommonAST(const unique_ptr<ASTNode>& node, int depth = 0);

void AST_test_main();

unique_ptr<ASTNode> AST_specified_node_construct(unique_ptr<NonTermProdNode> prodNode , AbstractSyntaxTree * ast_tree);


class ASTEnumTypeVisitor : public ASTVisitor {
int depth = 0;
public:
    bool moveSequence = false;

    inline void print(ASTNode * node) {
    std::cout<<ASTSubTypeToString(node->subType);
        if(dynamic_cast<ArithExpr *>(node)) {
            auto eptr = static_cast<ArithExpr *>(node);
            if(eptr->Op == BinaryOpEnum::ADD) {
                std::cout<<" +";
            } else if(eptr->Op == BinaryOpEnum::MUL) {
                std::cout<<" *";
            }
        }
        if(dynamic_cast<ConstExpr *>(node)) {
            std::cout<<" ";
            auto eptr = static_cast<ConstExpr *>(node);
            auto v = eptr->value;
            auto visitors = overload {
                [](int v) {std::cout<<v;},
                [](float v) {std::cout<<v;},
                [](uint64_t v) {std::cout<<v;},
            };
            std::visit(visitors,v);
        }
        if(dynamic_cast<SymIdNode *>(node)) {
            std::cout<<" ";
            auto eptr = static_cast<SymIdNode *>(node);
            std::cout<<toString_view(eptr->Literal);
            
        }
        std::cout<<std::endl;
    }

    virtual void visit(ASTNode* node) {
        if(moveSequence) {
            this->print(node); 
        }
        return;
    }
    inline virtual void enter(ASTNode* node) {
        
        if(!moveSequence) {
            for(int i = 0 ;i <depth;i++) {
                std::cout<<" ";
            }
            this->print(node); 
        }
        depth++;
        return;
    }
    inline virtual void quit(ASTNode* node) {
        depth--;
        return;
    }
};

class ConstantFoldingVisitor : public ASTVisitor {
public:
    std::stack<ASTNode *> ptr_stack;
    inline void enter(ASTNode * node) override {
        ptr_stack.push(node);
        return;
    }
    inline void quit(ASTNode * node) override{
        if(ptr_stack.empty()) {
            std::cerr<<"常量折叠visitor 栈错误"<<std::endl;
            return;
        }
        ptr_stack.pop();

        auto arithExpr_ptr = dynamic_cast<ArithExpr *>(node);
        if(!arithExpr_ptr) {
            return;
        }
        //常量折叠，要在quit时进行，因为会释放node在parent处的智能指针
        if(! (arithExpr_ptr->Lval_ptr->subType == ASTSubType::ConstExpr && arithExpr_ptr->Rval_ptr->subType == ASTSubType::ConstExpr) ) {
            //不满足常量折叠
            return;
        }
        auto newNode = std::make_unique<ConstExpr>();
        auto Lconst_ptr = static_cast<ConstExpr *>(arithExpr_ptr->Lval_ptr.get());
        auto Rconst_ptr = static_cast<ConstExpr *>(arithExpr_ptr->Rval_ptr.get());
        auto Fvisitor = overload {
            [](int v)->float {return v;},
            [](float v)->float {return v;},
            [](uint64_t v)->float {return v;},
        };
        auto Ivisitor = overload {
            [](int v)->int {return v;},
            [](float v)->int {return v;},
            [](uint64_t v)->int {return static_cast<int>(v);},
        };
        if(Lconst_ptr->Type.basicType == baseType::FLOAT || Rconst_ptr->Type.basicType == baseType::FLOAT) {
            float ans = std::visit(Fvisitor,Lconst_ptr->value);
            if(arithExpr_ptr->Op == BinaryOpEnum::ADD) {
                ans += std::visit(Fvisitor,Rconst_ptr->value);
            } else if (arithExpr_ptr->Op == BinaryOpEnum::MUL) {
                ans *= std::visit(Fvisitor,Rconst_ptr->value);
            }
            else {
                return;
            }
            newNode->Type = SymType();
            newNode->Type.basicType = baseType::FLOAT;
            newNode->value = ans;
        } else if(Lconst_ptr->Type.basicType == baseType::INT && Rconst_ptr->Type.basicType == baseType::INT){
            int ans = std::visit(Ivisitor,Lconst_ptr->value);
            if(arithExpr_ptr->Op == BinaryOpEnum::ADD) {
                ans += std::visit(Ivisitor,Rconst_ptr->value);
            } else if (arithExpr_ptr->Op == BinaryOpEnum::MUL) {
                ans *= std::visit(Ivisitor,Rconst_ptr->value);
            }
            else {
                return;
            }
            newNode->value = ans;
            newNode->Type = SymType();
            newNode->Type.basicType = baseType::INT;
        } else {
            return;
        }
        if(ptr_stack.empty()) {
            std::cerr<<"奇怪的AST栈，常量折叠Expr在AST顶"<<std::endl;
            return;
        }
        auto parent = ptr_stack.top();
        if(dynamic_cast<ArithExpr *>(parent)) {
            auto Ari = static_cast<ArithExpr *>(parent);
            if(Ari->Lval_ptr.get() == arithExpr_ptr) {
                Ari->Lval_ptr.reset(static_cast<Expr *>(newNode.release()));
            } else if(Ari->Rval_ptr.get() == arithExpr_ptr) {
                Ari->Rval_ptr.reset(static_cast<Expr *>(newNode.release()));
            } else {
                std::cerr<<"常量折叠碰到 奇怪的AST树栈结构"<<std::endl;
            }
        }
        else if(dynamic_cast<Assign *>(parent)) {
            auto assign = static_cast<Assign *>(parent);
            if(assign->expr_ptr.get() == arithExpr_ptr) {
                assign->expr_ptr.reset(static_cast<Expr *>(newNode.release()));
            } else {
                std::cerr<<"常量折叠碰到 奇怪的AST树栈结构"<<std::endl;
            }
        } else if (dynamic_cast<IdDeclare *>(parent)) {
            auto decl = static_cast<IdDeclare *>(parent);
            if(!decl->initExpr.has_value()) {
                std::cerr<<"常量折叠碰到 奇怪的AST树栈结构"<<std::endl;
                return;
            }
            if(decl->initExpr.value().get() == arithExpr_ptr) {
                decl->initExpr.value().reset(static_cast<Expr *>(newNode.release()));
            } else {
                std::cerr<<"常量折叠碰到 奇怪的AST树栈结构"<<std::endl;
            }
        }
        return;
    }
    inline void visit(ASTNode * node) override {
        return;
    }
};
} // namespace AST


#endif