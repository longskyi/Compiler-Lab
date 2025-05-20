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
};

class SymType
{
public:
    baseType basicType = NonInit;
    int array_len = 0;              //如果basic是array，则有该参数
    unique_ptr<SymType> eType = nullptr;    //是FUNC和FUNCPTR的RET类型 ， 是PTR的指向类型
    std::vector<unique_ptr<SymType>> TypeList;  //是FUNCPTR和FUNC的形参类型
    SymType() = default;
    inline SymType(SymType&& other) noexcept
        : basicType(other.basicType),
          array_len(other.array_len),
          eType(std::move(other.eType)),
          TypeList(std::move(other.TypeList)) {
        other.basicType = NonInit;
        other.array_len = 0;
    }
    inline SymType& operator=(SymType&& other) noexcept {
        if (this != &other) {
            basicType = other.basicType;
            array_len = other.array_len;
            eType = std::move(other.eType);
            TypeList = std::move(other.TypeList);

            other.basicType = NonInit;
            other.array_len = 0;
        }
        return *this;
    }
    inline SymType(const SymType& other) 
        : basicType(other.basicType),
          array_len(other.array_len) {
        // 深拷贝 eType
        if (other.eType) {
            eType = std::make_unique<SymType>(*other.eType);
        }
        
        // 深拷贝 TypeList 中的每个元素
        TypeList.reserve(other.TypeList.size());
        for (const auto& item : other.TypeList) {
            TypeList.push_back(std::make_unique<SymType>(*item));
        }
    }
    // 深拷贝赋值运算符
    inline SymType& operator=(const SymType& other) {
        if (this != &other) {
            // 拷贝基本成员
            basicType = other.basicType;
            array_len = other.array_len;
            
            // 深拷贝 eType
            if (other.eType) {
                eType = std::make_unique<SymType>(*other.eType);
            } else {
                eType.reset();
            }
            
            // 深拷贝 TypeList
            TypeList.clear();
            TypeList.reserve(other.TypeList.size());
            for (const auto& item : other.TypeList) {
                TypeList.push_back(std::make_unique<SymType>(*item));
            }
        }
        return *this;
    }
    inline size_t sizeoff() const {
        switch (basicType) {
            case INT:
                return 4;  // int32_t
            case FLOAT:
                return 4;  // 32位单精度浮点
            case VOID:
                return 0;  // void类型不占空间
            case BASE_PTR:
            case FUNC_PTR:
                return 8;  // 64位指针
            case ARRAY:
                if (eType && array_len > 0) {
                    return eType->sizeoff() * array_len;  // 递归计算数组元素大小
                }
                return 0;  // 无效数组
            case FUNC:
                return 0;  // 函数类型本身不占空间（函数指针才占空间）
            case NonInit:
            default:
                return 0;  // 未初始化类型
        }
    }
    inline size_t pointerStride() const {
        if (basicType == BASE_PTR || basicType == FUNC_PTR) {
            // 指针类型：步长由指向的类型决定
            if (eType) {
                return eType->sizeoff();  // 指向类型的大小
            }
            return 0;  // 无效指针类型
        }
        else if (basicType == ARRAY) {
            // 数组类型：退化为指针时的步长是元素大小
            if (eType) {
                return eType->sizeoff();
            }
            return 0;  // 无效数组类型
        }
        else {
            // 其他类型（INT/FLOAT等）的指针步长就是类型本身大小
            return sizeoff();
        }
    }
    inline bool check() const {
        switch (basicType) {
            case NonInit:
                return false;  // 未初始化的类型不合法

            case INT:
            case FLOAT:
            case VOID:
                // 基本类型不能有 eType 或 TypeList 或 array_len
                return (eType == nullptr) && TypeList.empty() && (array_len == 0);

            case BASE_PTR:
                // 基础指针必须有 eType 且 eType 本身合法
                // 不能有 TypeList 或 array_len
                return (eType != nullptr) && eType->check() && 
                    TypeList.empty() && (array_len == 0);

            case FUNC_PTR:
                // 函数指针必须有 eType (返回类型) 且合法
                // TypeList 中的每个参数类型必须合法
                // 不能有 array_len
                if (eType == nullptr || !eType->check() || array_len != 0) {
                    return false;
                }
                for (const auto& param : TypeList) {
                    if (param == nullptr || !param->check()) {
                        return false;
                    }
                }
                return true;

            case FUNC:
                // 函数类型必须有 eType (返回类型) 且合法
                // TypeList 中的每个参数类型必须合法
                // 不能有 array_len
                if (eType == nullptr || !eType->check() || array_len != 0) {
                    return false;
                }
                for (const auto& param : TypeList) {
                    if (param == nullptr || !param->check()) {
                        return false;
                    }
                }
                return true;

            case ARRAY:
                // 数组类型必须有 eType (元素类型) 且合法
                // array_len 必须 > 0
                // 不能有 TypeList
                return (eType != nullptr) && eType->check() && 
                    (array_len > 0) && TypeList.empty();

            default:
                return false;  // 未知类型不合法
        }
    }
    inline ~SymType(){}
    // inline SymType deref() {
    //     //return SymType(*this->eType);
    // }
    inline void makePtr() {
        auto NewEtype = std::make_unique<SymType>(std::move(*this));
        this->eType = std::move(NewEtype);
        this->TypeList.clear();
        this->basicType = BASE_PTR;
    }
    inline void makeFuncPtr(std::vector<unique_ptr<SymType>> TypeList_) {
        auto NewEtype = std::make_unique<SymType>(std::move(*this));
        this->eType = std::move(NewEtype);
        this->TypeList = std::move(TypeList_);
        this->basicType = FUNC_PTR;
    }
    inline void makeFunc(std::vector<unique_ptr<SymType>> TypeList_) {
        auto NewEtype = std::make_unique<SymType>(std::move(*this));
        this->eType = std::move(NewEtype);
        this->TypeList = std::move(TypeList_);
        this->basicType = FUNC;
    }
    inline void makeArray(int len) {
        auto NewEtype = std::make_unique<SymType>(std::move(*this));
        this->eType = std::move(NewEtype);
        this->TypeList.clear();
        this->basicType = ARRAY;
        this->array_len = len;
    }
    inline static bool equals(const SymType& a, const SymType& b) {
        if (a.basicType != b.basicType) {
            return false;
        }
        // 对于指针、数组和函数指针类型，需要递归比较指向的类型
        if (a.basicType == BASE_PTR || a.basicType == ARRAY || a.basicType == FUNC_PTR) {
            if(a.array_len!=b.array_len) {
                return false;
            }
            if ((a.eType == nullptr) != (b.eType == nullptr)) {
                return false;
            }
            if (a.eType && !equals(*a.eType, *b.eType)) {
                return false;
            }
        }
        // 对于函数指针类型，还需要比较参数类型列表
        if (a.basicType == FUNC_PTR || a.basicType == FUNC) {
            if (a.TypeList.size() != b.TypeList.size()) {
                return false;
            }
            for (size_t i = 0; i < a.TypeList.size(); ++i) {
                if (!equals(*a.TypeList[i], *b.TypeList[i])) {
                    return false;
                }
            }
        }
        return true;
    }
    inline std::u8string format() const {
        switch (basicType) {
            case INT: return u8"int";
            case FLOAT: return u8"float";
            case VOID: return u8"void";
            //case CHAR: return u8"char";
            
            case BASE_PTR: {
                if (!eType) return u8"void*";
                std::u8string inner = eType->format();
                // 处理函数指针的特殊情况
                if (eType->basicType == FUNC || eType->basicType == FUNC_PTR) {
                    return inner;  // 函数指针会在函数类型中处理
                }
                return inner + u8"*";
            }
            
            case ARRAY: {
                if (!eType) return u8"void[" + toU8str(std::to_string(array_len)) + u8"]";
                std::u8string inner = eType->format();
                // 处理数组指针的特殊情况
                if (eType->basicType == FUNC || eType->basicType == FUNC_PTR) {
                    return u8"(" + inner + u8")[" + toU8str(std::to_string(array_len)) + u8"]";
                }
                return inner + u8"[" + toU8str(std::to_string(array_len)) + u8"]";
            }
            case FUNC:
            case FUNC_PTR: {
                std::u8string result;
                // 处理返回类型
                if (eType) {
                    result = eType->format();
                } else {
                    result = u8"void";
                }
                // 添加指针标记（如果是函数指针）
                if (basicType == FUNC_PTR) {
                    result += u8"(*)";
                } else {
                    result += u8" ";
                }
                // 添加参数列表
                result += u8"(";
                for (size_t i = 0; i < TypeList.size(); ++i) {
                    if (i > 0) result += u8", ";
                    result += TypeList[i]->format();
                }
                result += u8")";
                
                return result;
            }
            
            default:
                return u8"unknown";
        }
    }
    inline std::tuple<bool, std::u8string> cast_to(const SymType* target) const {
        // 1. 检查空指针
        if (target == nullptr) {
            return {false, u8"内部错误，目标类型为空指针"};
        }
        // 2. 相同类型可以直接转换（无警告）
        if (equals(*this, *target)) {
            return {true, u8""};
        }
        // 3. void 指针的特殊处理（修正版）
        if (basicType == BASE_PTR && target->basicType == BASE_PTR) {
            // 先确保双方都是指针类型
            if (eType && eType->basicType == VOID) {
                return {true, u8""};  // void* 可以转换为任何指针类型
            }
            if (target->eType && target->eType->basicType == VOID) {
                return {true, u8""};  // 任何指针类型可以转换为 void*
            }
            // 非void指针之间的转换（可能有警告）
            return {true, u8"不同类型的指针转换可能有风险"};
        }
        // 4. 数值类型转换检查
        if ((basicType == INT || basicType == FLOAT) && 
            (target->basicType == INT || target->basicType == FLOAT)) {
            // 数值类型之间可以互相转换，但可能有精度损失警告
            if (basicType == FLOAT && target->basicType == INT) {
                return {true, u8"浮点数转换为整数可能导致精度损失"};
            }
            if (basicType == INT && target->basicType == FLOAT) {
                return {true, u8""};
            }
            return {true, u8""};
        }

        // 5. 数组和指针的转换
        if (basicType == ARRAY && target->basicType == BASE_PTR) {
            // 数组退化为指针，需要检查元素类型是否匹配
            if (eType && target->eType && equals(*eType, *target->eType)) {
                return {true, u8""};  // 类型匹配，无警告
            }
            return {false, u8"数组元素类型与指针指向类型不匹配"};
        }

        // 6. 函数指针转换
        if (basicType == FUNC_PTR && target->basicType == FUNC_PTR) {
            // 宽松的函数指针转换规则（可能有警告）
            return {true, u8"函数指针类型不完全匹配"};
        }

        // 如果以上条件都不满足，转换失败
        return {false, u8"类型不兼容，无法安全转换"};
    }
};

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


inline void test_typeSystem_main() {

    SymType intType;
    intType.basicType = INT;
    SymType floatType;
    floatType.basicType = FLOAT;

    SymType intPtrType;
    intPtrType.basicType = BASE_PTR;
    intPtrType.eType = std::make_unique<SymType>(intType);

    SymType intPtrPtrType;
    intPtrPtrType.basicType = BASE_PTR;
    intPtrPtrType.eType = std::make_unique<SymType>(intPtrType);

    SymType arrayType;
    arrayType.basicType = ARRAY;
    arrayType.array_len = 3;
    arrayType.eType = std::make_unique<SymType>(intType);
    SymType array2Type(arrayType);
    array2Type.makeArray(6);

    SymType funcType;
    funcType.basicType = FUNC;
    funcType.eType = std::make_unique<SymType>(intType);
    funcType.TypeList.push_back(std::make_unique<SymType>(intType));
    funcType.TypeList.push_back(std::make_unique<SymType>(floatType));

    SymType funcPtrType;
    funcPtrType.basicType = FUNC_PTR;
    funcPtrType.eType = std::make_unique<SymType>(intType);
    funcPtrType.TypeList.push_back(std::make_unique<SymType>(intType));
    funcPtrType.TypeList.push_back(std::make_unique<SymType>(floatType));

    // 预期输出:
    std::cout<<toString_view(intType.format());       // "int"
    std::cout<<std::endl;
    std::cout<<toString_view(intPtrType.format());    // "int*"
    std::cout<<std::endl;
    std::cout<<toString_view(intPtrPtrType.format()); // "int**"
    std::cout<<std::endl;
    std::cout<<toString_view(arrayType.format());     // "int[3]"
    std::cout<<std::endl;
    std::cout<<toString_view(array2Type.format());     // "int[3]"
    std::cout<<std::endl;
    arrayType.makePtr();
    array2Type.makePtr();
    std::cout<<toString_view(arrayType.format());     // "int(*)[3]"
    std::cout<<std::endl;
    std::cout<<toString_view(array2Type.format());     // "int(*)[3]"
    std::cout<<std::endl;
    std::cout<<toString_view(funcType.format());      // "int (int, float)"
    std::cout<<std::endl;
    std::cout<<toString_view(funcPtrType.format());   // "int(*)(int, float)"
    std::cout<<std::endl;
}


}

#endif