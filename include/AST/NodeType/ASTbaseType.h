#ifndef MAST_BASE_TYPE_HEADER
#define MAST_BASE_TYPE_HEADER
#include<memory>
#include<string>
#include<vector>
#include<SyntaxType.h>
#include<stringUtil.h>
namespace AST {


using u8string = std::u8string;

using std::unique_ptr;

class AbstractSyntaxTree;


enum baseType {
    NonInit,
    INT,
    FLOAT,
    VOID,
    FUNC,   //支持Param类型检查
    ARRAY,
    BASE_PTR,
    FUNC_PTR,   //不支持Param类型检查
    ARRAY_PTR,
};

class SymType
{
public:
baseType Type = NonInit;      // INT | FLOAT   
baseType eType = NonInit;     // BASE_PTR INT  | FUNC_PTR INT(函数返回类型)  //FUNC_PTR不计划支持param类型检查（过于复杂）
baseType eeType = NonInit;    // ARRAY_PTR BASE_PTR INT = int * d[]
    SymType(/* args */);
    ~SymType();
};

inline SymType::SymType(/* args */)
{
}

inline SymType::~SymType()
{
}


enum class ASTType {
    None,
    Common,
    TermSym,
    NonTermPordNode,

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
};

enum class ASTSubType {
    None,
    SubCommon,
    TermSym,
    NonTermPordNode,

    Expr,
    ConstExpr,
    DeferExpr,
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
};

class ASTVisitor;

class ASTNode
{
public:
    ASTType Ntype;
    ASTSubType subType;
    /**
     * @brief 允许调用visitor访问，不要针对visitor类型进行特化
     */
    virtual void accept(ASTVisitor& visitor) = 0; 
    /**
     * @brief 用于尝试构建的函数，构建成功返回智能指针，否则返回nullptr
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

//承载所有id绑定
class SymIdNode : public ASTNode
{
public :
    u8string Literal;
    void * symEntryPtr; //program symbol entry ptr
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
};


}

#endif