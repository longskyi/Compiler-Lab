#ifndef MAST_BASE_TYPE_HEADER
#define MAST_BASE_TYPE_HEADER
#include <cstddef>
#include <expected>
#include<memory>
#include<string>
#include<vector>
#include<SyntaxType.h>
#include<stringUtil.h>
#include"parserGen.h"
#include"TypingSystem.h"
namespace AST {


using u8string = std::u8string;

using std::unique_ptr;

class AbstractSyntaxTree;

enum class ASTType {
    None,
    Common,
    TermSym,
    NonTermPordNode,

    SymIdNode,

    Expr,
    ArgList,
    Arg,
    ASTBool,
    Declare,
    ParamList,
    Param,

    Program,
    BlockItemList,
    BlockItem,
    Block,

    Stmt,

    pType,

    pTypeList,
    Dimensions,
};

enum class ASTSubType {
    None,
    SubCommon,
    TermSym,
    NonTermPordNode,

    SymIdNode,

    Expr,
    ConstExpr,
    DerefExpr,
    CallExpr,
    ArithExpr,
    RightValueExpr,

    ArgList,
    Arg,

    ASTBool,

    Declare,
    IdDeclare,
    FuncDeclare,

    ParamList,
    Param,

    Program,
    BlockItemList,
    BlockItem,
    Block,

    Stmt,
    BlockStmt,
    Assign,
    Branch,
    Loop,
    FunctionCall,
    Return,

    pType,
    
    pTypeList,
    Dimensions,
};


const char* ASTTypeToString(ASTType type);
const char* ASTSubTypeToString(ASTSubType subtype);

class ASTVisitor;

class ASTNode
{
public:
    ASTType Ntype;
    ASTSubType subType;
    /**
     * @brief 允许调用visitor访问，实现自动递归的爬树，进入时调用enter，退出时调用quit，中间按照求值优先级顺序调用visit（包括自己）
     * @attention 不要考虑针对visitor类型的特化
     */
    inline virtual void accept(ASTVisitor& visitor) { return; }
    /**
     * @brief 用于尝试构建的函数，构建成功返回智能指针，否则返回nullptr
     * @attention 理论上这其实不必是一个虚函数，应该调用派生类的static函数
     */
    inline virtual unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) {
        return nullptr;
    }
    inline virtual void debug_print() {
        return;
    }
    inline ASTNode() {
        Ntype = ASTType::None;
        subType = ASTSubType::None;
    } 
    virtual ~ASTNode() = default;
};


class ASTVisitor {
public:

    virtual void visit(ASTNode* node) = 0;
    inline virtual void enter(ASTNode* node) =0;
    inline virtual void quit(ASTNode* node) = 0;
};


class ASTCommonNode : public ASTNode
{
public:
    inline ASTCommonNode() {
        Ntype = ASTType::Common;
        subType = ASTSubType::SubCommon;
    }
    inline bool isLeaf() {
        return childs.size() == 0;
    }
    inline void accept(ASTVisitor & visitor) override {
        //前序遍历
        visitor.enter(this);
        visitor.visit(this);
        for(auto & kid : childs) {
            kid->accept(visitor);
        }
        visitor.quit(this);
        return;
    }
    u8string type; //原始类型-非终结符
    u8string value;
    std::vector<unique_ptr<ASTNode>> childs;
};

//承载所有id绑定
class SymIdNode : public ASTNode
{
public :
    u8string Literal;
    void * symEntryPtr; //program symbol entry ptr
    inline SymIdNode() {
        this->Ntype = ASTType::SymIdNode;
        this->subType = ASTSubType::SymIdNode;
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        visitor.quit(this);
    }
};

/**
 * @brief 构建AST的通用节点，自动承载所有移进产生的节点（终结符节点）
 */
class TermSymNode : public ASTNode
{
public:
    std::u8string token_type;
    std::u8string value;
    TermSymNode(/* args */);
    ~TermSymNode();
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        visitor.quit(this);
        return;
    }
};

inline TermSymNode::TermSymNode(/* args */)
{
    this->Ntype = ASTType::TermSym;
    this->subType = ASTSubType::TermSym;
}

inline TermSymNode::~TermSymNode()
{
}

/**
 * @brief 构建AST的通用节点，自动承载所有归约产生的节点（非终结符节点），用于下一步try_construct
 */
class NonTermProdNode : public ASTNode
{
public:
    ProductionId prodId;    //归约产生式
    std::vector<unique_ptr<ASTNode>> childs;
    NonTermProdNode(/* args */);
    ~NonTermProdNode();
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        for(const auto & item: childs) {
            item->accept(visitor);
        }
        visitor.quit(this);
        return;
    }
};

inline NonTermProdNode::NonTermProdNode(/* args */)
{
    this->Ntype = ASTType::NonTermPordNode;
    this->subType = ASTSubType::NonTermPordNode;
}

inline NonTermProdNode::~NonTermProdNode()
{
}

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
    bool BuildSpecifiedAST(const std::vector<Lexer::scannerToken_t> & tokens);
    virtual ~AbstractSyntaxTree() = default;
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
    inline virtual void enter(ASTNode* node) override {
        return;
    }
    inline virtual void quit(ASTNode* node) override {
        return;
    }
};

}

#endif