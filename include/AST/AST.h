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

namespace AST
{

using u8string = std::u8string;

using std::unique_ptr;

extern StateId startStateId;

enum class ASTType {
    Common
};

enum class ASTSubType {
    SubCommon
};

class ASTVisitor;

class ASTNode
{
public:
    ASTType Ntype;
    ASTSubType subType;
    virtual void accept(ASTVisitor& visitor) = 0; 
    inline ASTNode() {
        Ntype = ASTType::Common;
        subType = ASTSubType::SubCommon;
    } 
    virtual ~ASTNode() = default;
};


class ASTVisitor {
public:
    virtual void visit(ASTNode* node) = 0;
};


class ASTCommonNode : public ASTNode
{
public:
    inline bool isLeaf() {
        return childs.size() == 0;
    }
    inline void accept(ASTVisitor & visitor) override {
        //前序遍历
        visitor.visit(this);
        for(auto & kid : childs) {
            kid->accept(visitor);
        }
        return;
    }
    u8string type; //原始类型-非终结符
    u8string value;
    std::vector<unique_ptr<ASTNode>> childs;
};

class mVisitor : public ASTVisitor {
    inline void visit(ASTNode * node) override {
        if(dynamic_cast<ASTCommonNode*>(node)) {
            this->visit(static_cast<ASTCommonNode*>(node));
        }
        return;
    }
    inline void visit(ASTCommonNode * node) {
        std::cerr << toString(node->value)<<" ";
        return;
    }
};

class AbstractSyntaxTree {
public:
    unique_ptr<ASTNode> root;
    const std::vector<std::vector<dotProdc>> states;
    const std::vector<std::unordered_map<SymbolId, StateId>> gotoTable;
    const std::vector<std::unordered_map<SymbolId, action>> actionTable;
    const SymbolTable symtab;
    const std::unordered_map<ProductionId, Production> Productions;
    AbstractSyntaxTree() = delete;
    inline AbstractSyntaxTree(ASTbaseContent inp)
     : states(std::move(inp.states)),gotoTable(std::move(inp.gotoTable)), actionTable(std::move(inp.actionTable)), symtab(std::move(inp.symtab)), Productions(std::move(inp.Productions))
    {
        root = nullptr;
    }
    bool BuildCommonAST(const std::vector<Lexer::scannerToken_t> & tokens);
    virtual ~AbstractSyntaxTree() = default;
};
    

void printCommonAST(const unique_ptr<ASTNode>& node, int depth = 0);

inline void test_main() {

    auto astbase = parserGen(
        u8"C:/code/CPP/Compiler-Lab/grammar/grammar.txt",
        u8"C:/code/CPP/Compiler-Lab/grammar/terminal.txt",
        u8"C:/code/CPP/Compiler-Lab/grammar/SLR1ConflictReslove.txt"
    );
    std::ofstream o("output.json");
    nlohmann::json j = astbase;
    o << std::setw(4) << j;  // std::setw(4) 控制缩进
    o.close();
    std::ifstream file("output.json");
    nlohmann::json j2;
    file >> j2;  // 从文件流解析
    ASTbaseContent astbase2 = j2;
    AST::AbstractSyntaxTree astT(astbase2);
    std::string myprogram = R"(
    int acc() {
    int a;
    int b;
    a = 10;
    b = 20;
    if (a < b) {
        return 1
    };
    return 0 
};
    )";
    auto ss = Lexer::scan(toU8str(myprogram));
    for(int i= 0 ;i < ss.size() ; i++) {
        auto q = ss[i];
        std::cout<<"["<<toString(q.type)<<" "<<toString(q.value)<<" "<<i<<"]";
    }
    std::cout<<"\n";
    astT.BuildCommonAST(ss);
    printCommonAST(astT.root);
    mVisitor v;
    astT.root->accept(v);
    std::cout<<std::endl;
    return;
    
}

} // namespace AST


#endif