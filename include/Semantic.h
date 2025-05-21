#ifndef M_LCMP_SEMANTIC_HEADER
#define M_LCMP_SEMANTIC_HEADER

//正常编译时的符号表
#include <cstdint>
#include <memory>
#include <vector>
#include <stack>
#include<unordered_map>
#include<unordered_set>
#include "AST/AST.h"

namespace Semantic {
using std::unique_ptr;

class SymbolEntry; 
class SemanticSymbolTable;

using symEntryID = StrongId<struct symEntryTag>;

class SymbolEntry {
public:
    inline static size_t next_index = 0;
    bool is_block = false;  //对于块作用域，没有名字，没有类型
    std::u8string symbolName;
    AST::SymType Type;
    int64_t offset = 0;
    int64_t alignment = 1;
    int64_t entrysize = 0;
    symEntryID symid;
    SymbolEntry();  //自动递增id
    unique_ptr<SemanticSymbolTable> subTable;
};

class SemanticSymbolTable {
public:
    std::u8string tabletag; //this is not a key;
    SemanticSymbolTable * outer = nullptr;
    int64_t width = 0;
    size_t abi_alignment = 16; // 向后续处理步骤提出的对齐要求
    std::vector<unique_ptr<SymbolEntry>> entries;
    //该函数不进行检查
    inline void add_entry(unique_ptr<SymbolEntry> entry) noexcept {
        entries.emplace_back(std::move(entry));
    }
    inline bool checkTyping() {
        bool ret = true;
        for(const auto & et : entries) {
            if(et->is_block) {
                ret = ret && et->subTable->checkTyping();
            }
            else if(et->Type.basicType == AST::FUNC) {
                ret =  ret && et->subTable->checkTyping();
            }
            else if(!et->Type.check()) {
                std::cerr<<"Symbol:"<<toString_view(et->symbolName)<<"不合法的类型"<<toString_view(et->Type.format())<<std::endl;
                ret = false;
            }
        }
        return ret;
    }
    inline void allocMem() {
        //填充offset和size字段
    }
    inline SymbolEntry * lookup(const std::u8string_view & name) {
        for(const auto & sym : entries) {
            if((!sym->is_block) && sym->symbolName == name) {
                return sym.get();
            }
        }
        if(outer) {
            return outer->lookup(name);
        }
        else {
            //查找失败
            return nullptr;
        }
    }
    inline SymbolEntry * lookupInner(const std::u8string_view & name) {
        for(const auto & sym : entries) {
            if((!sym->is_block) && sym->symbolName == name) {
                return sym.get();
            }
        }
        return nullptr;
    }
    inline void printTable(int depth = 0 , std::ostream& out = std::cout) {
        auto ptab = [depth](int a = 1){
            if(a == 0 )
                for(int i = 0 ; i< depth ; i ++)
                    std::cout<<"    "; // 4 spaces
            else
                for(int i = 0 ; i< depth +1 ; i ++)
                    std::cout<<"    "; // 4 spaces
        };
        ptab(0);
        out<<std::format("{}@Table[\n",toString_view(tabletag));
        ptab();
        out<<std::format("width:{} entryNum:{} alignment:{}\n",width,entries.size(),this->abi_alignment);
        for(int i = 0 ; i<entries.size();i++) {
            ptab();
            if(entries[i]->is_block) {
                out<<std::format("(type: block)\n");
                entries[i]->subTable->printTable(depth+1);
            } else if(entries[i]->Type.basicType == AST::FUNC) {
                out<<std::format("(name:{},uniqueId:{},type: FUNC[{}])\n",toString_view(entries[i]->symbolName),size_t(entries[i]->symid),toString_view(entries[i]->Type.format()));
                entries[i]->subTable->printTable(depth+1);
            }
            else {
                out<<std::format("(name:{},uniqueId:{},type:{},sizeof:{})\n",toString_view(entries[i]->symbolName),size_t(entries[i]->symid),toString_view(entries[i]->Type.format()),entries[i]->Type.sizeoff());
            }
        }
        ptab(0);
        out<<"]"<<std::endl;
    }

};

inline SymbolEntry::SymbolEntry() : symid(symEntryID(next_index++)) {}


//构造符号表
class SymbolTableBuildVisitor : public AST::ASTVisitor {
public:
    SemanticSymbolTable * rootTable;
    SemanticSymbolTable * currentTable;
    AST::AbstractSyntaxTree * AStree;
    bool gotError = false;
    std::stack<AST::ASTNode*> defineStack;
    //需要关注 block，以及函数定义-block联合体

    void enter(AST::ASTNode* node) {
        if(gotError) return;
        if(dynamic_cast<AST::FuncDeclare *>(node)) {
            if(currentTable->outer != nullptr) {
                std::cerr<<std::format("在函数内定义函数未受支持\n");
                gotError = true;
            }
            auto funcD_ptr = static_cast<AST::FuncDeclare *>(node);
            if(currentTable->lookupInner(funcD_ptr->id_ptr->Literal)) {
                std::cerr<<toString_view(funcD_ptr->id_ptr->Literal)<<"多重定义错误"<<std::endl;
                gotError = true;
            }
            if(!funcD_ptr) {
                std::cerr<<"函数-block定义栈结构损坏"<<std::endl;
                gotError = true;
                return;
            }
            auto entry_ptr = std::make_unique<SymbolEntry>();
            auto table_ptr = std::make_unique<SemanticSymbolTable>();
            table_ptr->outer = currentTable;
            table_ptr->tabletag = funcD_ptr->id_ptr->Literal;
            entry_ptr->symbolName = funcD_ptr->id_ptr->Literal;
            entry_ptr->Type = funcD_ptr->funcType;
            auto tmp_ptr = entry_ptr.get();
            currentTable->add_entry(std::move(entry_ptr));
            currentTable = table_ptr.get();
            tmp_ptr->subTable = std::move(table_ptr);
            funcD_ptr->id_ptr->symEntryPtr = tmp_ptr;
            InnerState = 1;
        }
        if(dynamic_cast<AST::Block *>(node)) {
            if(InnerState == 1) {
                //函数定义-block 联合体
                auto block_ptr = static_cast<AST::Block*>(node);
                block_ptr->symTable = currentTable;
                InnerState = 0;
            } else if(InnerState == 0 ) {
                //block独立出现
                auto entry_ptr = std::make_unique<SymbolEntry>();
                auto table_ptr = std::make_unique<SemanticSymbolTable>();
                //block 完成作用域绑定
                auto block_ptr = static_cast<AST::Block *>(node);
                block_ptr->symTable = table_ptr.get();

                table_ptr->outer = currentTable;
                entry_ptr->is_block = true;
                auto tmp_ptr = entry_ptr.get();
                currentTable->add_entry(std::move(entry_ptr));
                currentTable = table_ptr.get();
                tmp_ptr->subTable = std::move(table_ptr);
                
            } else {
                std::cerr<<"Inner ERROR Not Implement states"<<std::endl;
            }
        }

        defineStack.push(node);
    }
    int InnerState = 0;
    void visit(AST::ASTNode* node) {
        if(gotError) return;
        if(dynamic_cast<AST::IdDeclare *>(node)) {
            auto idDecl_ptr = static_cast<AST::IdDeclare *>(node);
            const std::u8string_view literal_view = idDecl_ptr->id_ptr->Literal;
            if(currentTable->lookupInner(literal_view)) {
                std::cerr<<toString_view(literal_view)<<"多重定义错误"<<std::endl;
                gotError = true;
            } else {
                auto entry_ptr = std::make_unique<SymbolEntry>();
                entry_ptr->symbolName = idDecl_ptr->id_ptr->Literal;
                entry_ptr->Type = idDecl_ptr ->id_type;
                idDecl_ptr->id_ptr->symEntryPtr = entry_ptr.get();
                currentTable->add_entry(std::move(entry_ptr));
            }
        }
        if(dynamic_cast<AST::Arg *>(node)) {
            auto arg_ptr = static_cast<AST::Arg *>(node);
            const std::u8string_view literal_view = arg_ptr->id_ptr->Literal;
            if(currentTable->lookupInner(literal_view)) {
                std::cerr<<toString_view(literal_view)<<"多重定义错误"<<std::endl;
                gotError = true;
            } else {
                auto entry_ptr = std::make_unique<SymbolEntry>();
                entry_ptr->symbolName = arg_ptr->id_ptr->Literal;
                entry_ptr->Type = arg_ptr->argtype;
                arg_ptr->id_ptr->symEntryPtr = entry_ptr.get();
                currentTable->add_entry(std::move(entry_ptr));
            }
        }
    }
    void quit(AST::ASTNode* node) {
        if(gotError) return;
        if(defineStack.empty()) {
            std::cerr<<"符号表生成 栈深度结构损坏"<<std::endl;
            gotError = true;
            return;
        }
        defineStack.pop();
        if(dynamic_cast<AST::SymIdNode *>(node)) {
            auto idNode = static_cast<AST::SymIdNode *>(node);
            auto parent = defineStack.top();
            //进行符号查询绑定的id，其父节点为 Expr或 Stmt
            if(dynamic_cast<AST::Expr *>(parent)) {
                auto Se = currentTable->lookup(idNode->Literal);
                if(Se == nullptr) {
                    std::cerr<<std::format("未定义符号{}\n",toString_view(idNode->Literal));
                    gotError = true;
                    return;
                }
                idNode->symEntryPtr = Se;
            }   
            else if(dynamic_cast<AST::Stmt *>(parent)) {
                auto Se = currentTable->lookup(idNode->Literal);
                if(Se == nullptr) {
                    std::cerr<<std::format("未定义符号{}\n",toString_view(idNode->Literal));
                    gotError = true;
                    return;
                }
                idNode->symEntryPtr = Se;
            }
        }
        if(dynamic_cast<AST::Block *>(node)) {
            if(defineStack.empty()) {
                std::cerr<<"符号表生成 栈结构损坏"<<std::endl;
                gotError = true;
                return;
            }
            currentTable = currentTable->outer;
        }
    }

    bool build(SemanticSymbolTable * root , AST::AbstractSyntaxTree * tree) {
        defineStack = std::stack<AST::ASTNode*>();
        InnerState = 0;
        if(root == nullptr) {
            std::cerr<<"符号表生成输入表为空指针"<<std::endl;
            return false;
        }
        if(tree == nullptr) {
            std::cerr<<"符号表生成输入树为空指针"<<std::endl;
            return false;
        }
        rootTable = root;
        currentTable = root;
        AStree = tree;
        AStree->root->accept(*this);
        if(gotError) {
            std::cerr<<"符号表生成出现错误"<<std::endl;
        }
        return gotError;
    }
};


enum ArithOp {
    INVALID,
    ADDI,
    ADDF,
    MULI,
    MULF,
    PTR_ADDI,
    ADDU,
};

//类型检查
//类型检查生成的节点内容
struct TypingCheckNode {
    AST::SymType retType;               //初始化必填字段
    AST::CAST_OP cast_op = AST::NO_OP;  
    AST::SymType castType;
    bool ret_is_left_value = false; //作为左值使用
    ArithOp arithOp = INVALID;  //后续应该会需要
    int helpNum1 = 0;
    int helpNum2 = 0;
    void * otherContent = nullptr;
};

//类型检查内容

/**

Bool -> Expr rop Expr
检查可比较，比较方法 cmpI cmpU cmpF

Expr op Expr
检查可计算，修改子Expr1 Expr2的标记为右值

* Expr
标记返回可为左值

id = Expr
左右类型检查，左侧左值检查，右侧标记为右值

Expr = Expr;
左右类型检查，左侧左值检查，右侧标记为右值

Expr -> id ( ParamList )
函数Param与Arg检测

Stmt -> id ( ParamList )
函数Param与Arg检测
返回值不为空时警告

Type id = Expr ;
左右类型检查

FuncDecalre -> Block -> return Expr ; | return ;
检测返回类型正确性

// 所有分支都有return - 不属于类型检查


*/





using ExprTypeCheckMap = std::unordered_map<AST::Expr *,TypingCheckNode>;
using ArgTypeCheckMap = std::unordered_map<AST::Arg *,TypingCheckNode>;


inline bool parseExprCheck(AST::Expr * mainExpr_ptr , ExprTypeCheckMap & ExprMap) {
    if(mainExpr_ptr == nullptr) {
        std::cout<<std::format("Expr检查失败，空指针错误\n");
        return false;
    }
    if(dynamic_cast<AST::ConstExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::ConstExpr *>(mainExpr_ptr);
        TypingCheckNode currNode;
        currNode.retType = Expr_ptr->Type;
        ExprMap[Expr_ptr] = std::move(currNode);
    }
    if(dynamic_cast<AST::DerefExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::DerefExpr *>(mainExpr_ptr);
        auto subExpr_ptr = Expr_ptr->subExpr.get();
        auto subTyping = ExprMap.at(subExpr_ptr);
        auto ret = subTyping.retType.deref();
        if(!ret) {
            std::cerr<<std::format("对类型{}的解引用失败\n",toString_view(subTyping.retType.format()));
            auto Error = ret.error();
            //AST::SymType::
            return false;
        }
        auto [canBeLval,Symstype] = ret.value();
        //如果被解引用后成了左值，那么被解引用前被视为右值
        ExprMap.at(subExpr_ptr).ret_is_left_value = ExprMap.at(subExpr_ptr).ret_is_left_value && !canBeLval;
        TypingCheckNode currNode;
        currNode.retType = std::move(Symstype);
        currNode.ret_is_left_value = canBeLval;
        ExprMap[Expr_ptr] = std::move(currNode);
    }
    if(dynamic_cast<AST::CallExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::CallExpr *>(mainExpr_ptr);
        SymbolEntry * Se_ptr = static_cast<SymbolEntry *>(Expr_ptr->id_ptr->symEntryPtr);
        if(Se_ptr == nullptr) {
            std::cerr<<"内部错误，符号没有被绑定"<<std::endl;
            return false;
        }
        const auto & idType = Se_ptr->Type;
        if(idType.basicType != AST::baseType::FUNC && idType.basicType != AST::baseType::FUNC_PTR) {
            std::cerr<<
            std::format("错误，变量{} 类型{}不是函数类型",
                toString_view(Se_ptr->symbolName),toString_view(idType.format()));
            return false;
        }
        /**
        
        重要：此处只实现 Param -> Expr支持，足以覆盖原有文法
        
        */
        const auto & paramL = Expr_ptr->ParamList_ptr->paramList;
        if(paramL.size() != idType.TypeList.size() && idType.TypeList.size()!=0) {
            std::cerr<<std::format("函数{}实参形参数量不匹配",toString_view(Se_ptr->symbolName));
            return false;
        }

        for(int i = 0 ; i< paramL.size() && idType.TypeList.size()!=0 ;i++) {
            // Expr -> id( paramLists ) -> param -> Expr
            const auto & paramT = paramL[i];
            const auto & argType = idType.TypeList[i].get();
            const auto paramExpr_ptr = paramT->expr_ptr.get();
            //忽略 id_ptr 和 Op
            auto & paramTypeNode = ExprMap.at(paramExpr_ptr);
            auto [cast_op,msg] = paramTypeNode.retType.cast_to(argType);
            if(cast_op == AST::CAST_OP::INVALID) {
                std::cerr<<std::format("函数调用，类型转换失败，源类型{}，无法转换成{}\n",
                    toString_view(paramTypeNode.retType.format()),
                    toString_view(argType->format())
                );
                return false;
            }
            else {
                if(!msg.empty()) {
                    std::cerr<<"类型转换警告："<<toString_view(msg)<<std::endl;
                }
                paramTypeNode.castType = AST::SymType(*argType);
                paramTypeNode.cast_op = cast_op;
            }
        }
        TypingCheckNode currNode;
        currNode.retType = *(idType.eType.get());
        currNode.ret_is_left_value = false;
        ExprMap[Expr_ptr] = std::move(currNode);
    }
    if(dynamic_cast<AST::ArithExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::ArithExpr *>(mainExpr_ptr);
        /**
            INVALID,
            ADDI,
            ADDF,
            MULI,
            MULF,
            PTR_ADDI,   //必须是PTR + int，修改 helpNum1为sizeoff
            ADDU,   //暂时不用
            //指针加法不被允许，但是指针减法是存在的，暂时没有实现
        */
        auto Lexpr_ptr = Expr_ptr->Lval_ptr.get();
        auto Rexpr_ptr =Expr_ptr->Rval_ptr.get();
        auto & Lexpr_Typing_Node = ExprMap.at(Lexpr_ptr);
        auto & Rexpr_Typing_Node = ExprMap.at(Rexpr_ptr);
        const auto & Ltype = Lexpr_Typing_Node.retType;
        const auto & Rtype = Rexpr_Typing_Node.retType;
        if(Expr_ptr->Op == AST::ADD) 
        {
            if(Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::INT) {
            //ADDI
            TypingCheckNode currNode;
            currNode.retType = Lexpr_Typing_Node.retType;
            currNode.ret_is_left_value = false;
            currNode.arithOp = ADDI;
            ExprMap[Expr_ptr] = std::move(currNode);
            }
            else if( (Ltype.basicType == AST::baseType::FLOAT && Rtype.basicType == AST::baseType::INT) ||
                    (Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::FLOAT) || 
                    (Ltype.basicType == AST::baseType::FLOAT && Rtype.basicType == AST::baseType::FLOAT)) 
            {
                //ADDF
                TypingCheckNode currNode;
                currNode.retType = Lexpr_Typing_Node.retType;
                currNode.ret_is_left_value = false;
                currNode.arithOp = ADDF;
                AST::SymType floatType;
                floatType.basicType = AST::baseType::FLOAT;
                ExprMap[Expr_ptr] = std::move(currNode);
                if(Lexpr_Typing_Node.retType.basicType == AST::baseType::INT) {
                    Lexpr_Typing_Node.cast_op = AST::CAST_OP::INT_TO_FLOAT;
                    Lexpr_Typing_Node.castType = floatType;
                }
                if(Rexpr_Typing_Node.retType.basicType == AST::baseType::INT) {
                    Rexpr_Typing_Node.cast_op = AST::CAST_OP::INT_TO_FLOAT;
                    Rexpr_Typing_Node.castType = floatType;
                }
            }
            else if((Ltype.basicType == AST::baseType::BASE_PTR && Rtype.basicType == AST::baseType::INT) ||
                    (Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::BASE_PTR)) {
                //PTR ADDI
                TypingCheckNode * ptrExprTyping;
                TypingCheckNode * intExprTyping;
                if(Ltype.basicType == AST::baseType::BASE_PTR) {
                    ptrExprTyping = &Lexpr_Typing_Node;
                    intExprTyping = &Rexpr_Typing_Node;
                } else {
                    ptrExprTyping = &Rexpr_Typing_Node;
                    intExprTyping = &Lexpr_Typing_Node;                    
                }
                int ptrStepLen = ptrExprTyping->retType.pointerStride();
                if(ptrStepLen == 0 ){
                    std::cerr<<std::format("指针类型 {} + 运算不被允许\n",
                    toString_view(ptrExprTyping->retType.format()));
                    return false;
                }
                else {
                    TypingCheckNode currNode;
                    currNode.retType = ptrExprTyping->retType;
                    currNode.ret_is_left_value = false;
                    currNode.arithOp = PTR_ADDI;
                    currNode.helpNum1 = ptrStepLen;
                    ExprMap[Expr_ptr] = std::move(currNode);
                }
            }else if((Ltype.basicType == AST::baseType::ARRAY && Rtype.basicType == AST::baseType::INT) ||
                    (Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::ARRAY)) {
                //PTR ADDI
                TypingCheckNode * ptrExprTyping;
                TypingCheckNode * intExprTyping;
                if(Ltype.basicType == AST::baseType::ARRAY) {
                    ptrExprTyping = &Lexpr_Typing_Node;
                    intExprTyping = &Rexpr_Typing_Node;
                } else {
                    ptrExprTyping = &Rexpr_Typing_Node;
                    intExprTyping = &Lexpr_Typing_Node;                    
                }
                ptrExprTyping->castType = ptrExprTyping->retType;
                ptrExprTyping->cast_op = AST::ARRAY_TO_PTR;
                ptrExprTyping->castType.basicType = AST::BASE_PTR;
                ptrExprTyping->castType.array_len = 0;
                int ptrStepLen = ptrExprTyping->retType.pointerStride();
                if(ptrStepLen == 0 ){
                    std::cerr<<std::format("指针类型 {} + 运算不被允许\n",
                    toString_view(ptrExprTyping->retType.format()));
                    return false;
                }
                else {
                    TypingCheckNode currNode;
                    currNode.retType = ptrExprTyping->retType;
                    currNode.ret_is_left_value = false;
                    currNode.arithOp = PTR_ADDI;
                    currNode.helpNum1 = ptrStepLen;
                    ExprMap[Expr_ptr] = std::move(currNode);
                }
            } else {
                std::cerr<<std::format("类型 {} [+] {} 不支持 + 运算符\n",
                    toString_view(Ltype.format()),toString_view(Rtype.format()));
                return false;
            }
        } else if(Expr_ptr->Op == AST::MUL) {
            if(Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::INT) {
                // MULI
                TypingCheckNode currNode;
                currNode.retType = Lexpr_Typing_Node.retType;
                currNode.ret_is_left_value = false;
                currNode.arithOp = MULI;
                ExprMap[Expr_ptr] = std::move(currNode);
            }
            else if( (Ltype.basicType == AST::baseType::FLOAT && Rtype.basicType == AST::baseType::INT) ||
                    (Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::FLOAT) || 
                    (Ltype.basicType == AST::baseType::FLOAT && Rtype.basicType == AST::baseType::FLOAT)) 
            {
                // MULF
                TypingCheckNode currNode;
                currNode.retType.basicType = AST::baseType::FLOAT;
                currNode.ret_is_left_value = false;
                currNode.arithOp = MULF;
                AST::SymType floatType;
                floatType.basicType = AST::baseType::FLOAT;
                ExprMap[Expr_ptr] = std::move(currNode);
                if(Lexpr_Typing_Node.retType.basicType == AST::baseType::INT) {
                    Lexpr_Typing_Node.cast_op = AST::CAST_OP::INT_TO_FLOAT;
                    Lexpr_Typing_Node.castType = floatType;
                }
                if(Rexpr_Typing_Node.retType.basicType == AST::baseType::INT) {
                    Rexpr_Typing_Node.cast_op = AST::CAST_OP::INT_TO_FLOAT;
                    Rexpr_Typing_Node.castType = floatType;
                }
            } else {
                std::cerr<<std::format("类型 {} [*] {} 不支持 * 运算符\n",
                    toString_view(Ltype.format()),toString_view(Rtype.format()));
                return false;
            }
        } else {
            std::cerr<<"运算符Not implement"<<std::endl;
            return false;
        }
        
    }
    if(dynamic_cast<AST::IdValueExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::IdValueExpr *>(mainExpr_ptr);
        auto behave = Expr_ptr->behave;
        auto id_ptr = Expr_ptr->id_ptr.get();
        auto SE_ptr = static_cast<SymbolEntry *> (id_ptr->symEntryPtr);
        if(behave == AST::direct) {
            //直接访问
            //要尽可能保留类型，直到更上层的隐式类型转换
            auto & idType = SE_ptr->Type;
            TypingCheckNode currNode;
            currNode.retType = idType;
            if(idType.basicType == AST::FUNC || idType.basicType == AST::FUNC_PTR || idType.basicType == AST::ARRAY) {
                currNode.ret_is_left_value = false;
            } else {
                currNode.ret_is_left_value = true;
            }
            ExprMap[Expr_ptr] = std::move(currNode);
        } else if(behave == AST::ref) {
            //取地址
            auto & idType = SE_ptr->Type;
            TypingCheckNode currNode;
            currNode.retType = idType;
            currNode.retType.makePtr();
            currNode.ret_is_left_value = true;
            ExprMap[Expr_ptr] = std::move(currNode);
        } else {
            std::cerr<<"内部错误，Id behave未初始化"<<std::endl;
            return false;
        }
        
    }

    return true;
}

//不包含return检查
inline bool parseStmtCheck(AST::Stmt * mainStmt_ptr, ExprTypeCheckMap & ExprMap) {
    if(mainStmt_ptr == nullptr) {
        std::cout<<std::format("Stmt检查失败，空指针错误\n");
        return false;
    }
    if(dynamic_cast<AST::FunctionCall *>(mainStmt_ptr)) {
        auto Stmt_ptr = static_cast<AST::FunctionCall *>(mainStmt_ptr);
        SymbolEntry * Se_ptr = static_cast<SymbolEntry *>(Stmt_ptr->id_ptr->symEntryPtr);
        if(Se_ptr == nullptr) {
            std::cerr<<"内部错误，符号没有被绑定"<<std::endl;
            return false;
        }
        const auto & idType = Se_ptr->Type;
        if(idType.basicType != AST::baseType::FUNC && idType.basicType != AST::baseType::FUNC_PTR) {
            std::cerr<<
            std::format("错误：变量{} 类型{}不是函数类型\n",
                toString_view(Se_ptr->symbolName),toString_view(idType.format()));
            return false;
        }
        if(idType.eType->basicType != AST::baseType::VOID) {
            std::cerr<<"警告："<<toString_view(Se_ptr->symbolName)<<"函数返回值未被使用\n";
        }
        /**
        
        重要：此处只实现 Param -> Expr支持，足以覆盖原有文法
        
        */
        const auto & paramL = Stmt_ptr->paramList_ptr->paramList;
        if(paramL.size() != idType.TypeList.size()) {
            std::cerr<<std::format("函数{}实参形参数量不匹配",toString_view(Se_ptr->symbolName));
            return false;
        }

        for(int i = 0 ; i< paramL.size();i++) {
            const auto & paramT = paramL[i];
            const auto & argType = idType.TypeList[i].get();
            const auto paramExpr_ptr = paramT->expr_ptr.get();
            //忽略 id_ptr 和 Op
            auto & paramTypeNode = ExprMap.at(paramExpr_ptr);
            auto [cast_op,msg] = paramTypeNode.retType.cast_to(argType);
            if(cast_op == AST::CAST_OP::INVALID) {
                std::cerr<<std::format("函数调用，类型转换失败，源类型{}，无法转换成{}\n",
                    toString_view(paramTypeNode.retType.format()),
                    toString_view(argType->format())
                );
                return false;
            }
            else {
                paramTypeNode.castType = AST::SymType(*argType);
                paramTypeNode.cast_op = cast_op;
            }
        }
    }
    if(dynamic_cast<AST::Assign *>(mainStmt_ptr)) {
        auto Stmt_ptr = static_cast<AST::Assign *>(mainStmt_ptr);
        auto RNode = ExprMap.at(Stmt_ptr->Rexpr_ptr.get());
        if(Stmt_ptr->Lexpr_ptr) {
            auto LNode = ExprMap.at(Stmt_ptr->Lexpr_ptr.get());
            if(!LNode.ret_is_left_value) {
                std::cerr<<"=左侧不能是右值"<<std::endl;
                return false;
            }
            RNode.ret_is_left_value = false;
            if(AST::SymType::equals(LNode.retType,RNode.retType)) {

            }
            else {
                auto [cast_op , msg] = RNode.retType.cast_to(&LNode.retType);
                if(cast_op == AST::CAST_OP::INVALID) {
                    std::cerr<<std::format("assign，类型转换失败，源类型{}，无法转换成{}\n",
                        toString_view(RNode.retType.format()),
                        toString_view(LNode.retType.format())
                    );
                    return false;
                }
                else {
                    if(!msg.empty()) {
                        std::cerr<<"类型转换警告："<<toString_view(msg)<<std::endl;
                    }
                    RNode.castType = AST::SymType(LNode.retType);
                    RNode.cast_op = cast_op;
                }
            }
        } else if(Stmt_ptr->id_ptr) {
            auto Se_ptr =  static_cast<SymbolEntry*>(Stmt_ptr->id_ptr->symEntryPtr) ;
            auto Ltype = Se_ptr->Type;
            bool L_is_left_value = true;
            if(Ltype.basicType == AST::FUNC || Ltype.basicType == AST::ARRAY) {
                L_is_left_value = false;
            }
            if(!L_is_left_value) {
                std::cerr<<"=左侧不能是右值"<<std::endl;
                return false;
            }
            RNode.ret_is_left_value = false;
            if(AST::SymType::equals(Ltype,RNode.retType)) {

            }
            else {
                auto [cast_op , msg] = RNode.retType.cast_to(&Ltype);
                if(cast_op == AST::CAST_OP::INVALID) {
                    std::cerr<<std::format("assign，类型转换失败，源类型{}，无法转换成{}\n",
                        toString_view(RNode.retType.format()),
                        toString_view(Ltype.format())
                    );
                    return false;
                }
                else {
                    if(!msg.empty()) {
                        std::cerr<<"类型转换警告："<<toString_view(msg)<<std::endl;
                    }
                    RNode.castType = AST::SymType(Ltype);
                    RNode.cast_op = cast_op;
                }
            }
        } else {
            std::cerr<<"内部错误，受损的Assign结点";
            return false;
        }
    }
    return true;
}

inline bool parseBoolCheck(AST::ASTBool * ASTbool_ptr , ExprTypeCheckMap & ExprMap) {
    if(ASTbool_ptr == nullptr) {
        std::cout<<std::format("Bool检查失败，空指针错误\n");
        return false;
    }
    
    auto Lval = ASTbool_ptr->Lval_ptr.get();
    auto& Lnode = ExprMap[Lval];
    
    // 1. 处理无比较运算符的情况（直接转换为bool）
    if(ASTbool_ptr->rop == AST::NoneROP) {
        Lnode.ret_is_left_value = false; // 作为条件表达式总是右值
        
        // 检查类型是否可转换为bool
        switch(Lnode.retType.basicType) {
            case AST::VOID:
                std::cerr << std::format("错误：void类型不能转换为bool\n");
                return false;
            case AST::ARRAY:
                // 数组退化为指针
                Lnode.castType = Lnode.retType;
                Lnode.castType.basicType = AST::BASE_PTR;
                Lnode.castType.array_len = 0;
                Lnode.cast_op = AST::ARRAY_TO_PTR;
                break;
            case AST::FUNC:
                // 函数退化为函数指针
                Lnode.castType = Lnode.retType;
                Lnode.castType.basicType = AST::FUNC_PTR;
                Lnode.cast_op = AST::FUNC_TO_PTR;
                break;
            default: // INT, FLOAT, PTR等类型可以直接作为bool
                break;
        }
        return true;
    }
    
    // 2. 处理有比较运算符的情况
    auto Rval = ASTbool_ptr->Rval_ptr.get();
    auto& Rnode = ExprMap[Rval];
    
    // 两边都不能是void
    if(Lnode.retType.basicType == AST::VOID || Rnode.retType.basicType == AST::VOID) {
        std::cerr << std::format("错误：void类型不能参与比较\n");
        return false;
    }
    
    // 处理数组退化
    if(Lnode.retType.basicType == AST::ARRAY) {
        Lnode.castType = Lnode.retType;
        Lnode.castType.basicType = AST::BASE_PTR;
        Lnode.castType.array_len = 0;
        Lnode.cast_op = AST::ARRAY_TO_PTR;
    }
    if(Rnode.retType.basicType == AST::ARRAY) {
        Rnode.castType = Rnode.retType;
        Rnode.castType.basicType = AST::BASE_PTR;
        Rnode.castType.array_len = 0;
        Rnode.cast_op = AST::ARRAY_TO_PTR;
    }
    
    // 处理函数退化
    if(Lnode.retType.basicType == AST::FUNC) {
        Lnode.castType = Lnode.retType;
        Lnode.castType.basicType = AST::FUNC_PTR;
        Lnode.cast_op = AST::FUNC_TO_PTR;
    }
    if(Rnode.retType.basicType == AST::FUNC) {
        Rnode.castType = Rnode.retType;
        Rnode.castType.basicType = AST::FUNC_PTR;
        Rnode.cast_op = AST::FUNC_TO_PTR;
    }
    
    // 获取实际比较类型（考虑可能的转换后类型）
    const auto& Ltype = (Lnode.cast_op != AST::NO_OP) ? Lnode.castType : Lnode.retType;
    const auto& Rtype = (Rnode.cast_op != AST::NO_OP) ? Rnode.castType : Rnode.retType;
    
    // 检查类型是否可比较
    if(Ltype.basicType == AST::FLOAT || Rtype.basicType == AST::FLOAT) {
        // 涉及float的比较
        if(Ltype.basicType != AST::FLOAT && Ltype.basicType != AST::INT) {
            std::cerr << std::format("错误：类型{}不能与float比较\n", toString_view(Ltype.format()));
            return false;
        }
        if(Rtype.basicType != AST::FLOAT && Rtype.basicType != AST::INT) {
            std::cerr << std::format("错误：类型{}不能与float比较\n", toString_view(Rtype.format()));
            return false;
        }
        
        // 需要int转float
        if(Ltype.basicType == AST::INT) {
            Lnode.castType.basicType = AST::FLOAT;
            Lnode.cast_op = AST::INT_TO_FLOAT;
        }
        if(Rtype.basicType == AST::INT) {
            Rnode.castType.basicType = AST::FLOAT;
            Rnode.cast_op = AST::INT_TO_FLOAT;
        }
    } 
    else if(Ltype.basicType == AST::BASE_PTR || Rtype.basicType == AST::BASE_PTR) {
        // 指针比较
        if(Ltype.basicType != AST::BASE_PTR || Rtype.basicType != AST::BASE_PTR) {
            std::cerr << std::format("错误：指针只能与指针比较\n");
            return false;
        }
        
        // 检查指针类型是否兼容
        if(!AST::SymType::equals(Ltype, Rtype)) {
            std::cerr << std::format("错误：不兼容的指针类型比较: {} 和 {}\n", 
                toString_view(Ltype.format()), toString_view(Rtype.format()));
            return false;
        }
    }
    else if(Ltype.basicType != Rtype.basicType) {
        std::cerr << std::format("错误：不兼容的类型比较: {} 和 {}\n", 
            toString_view(Ltype.format()), toString_view(Rtype.format()));
        return false;
    }
    
    // 标记为右值
    Lnode.ret_is_left_value = false;
    Rnode.ret_is_left_value = false;
    
    return true;
}

inline bool parseReturnCheck(AST::Return * return_ptr , SemanticSymbolTable * rootTable ,AST::FuncDeclare * func_ptr, ExprTypeCheckMap & ExprMap) {
    auto & funcName = func_ptr->id_ptr->Literal;
    auto Se_ptr = rootTable->lookup(funcName);
    if(Se_ptr == nullptr) {
        std::cerr<<"函数return检查失败 损坏的符号表\n";
        return false;
    }
    auto retType = *Se_ptr->Type.eType.get();
    if(!return_ptr->expr_ptr.has_value()) {
        if(retType.basicType != AST::baseType::VOID) {
            std::cerr<<"函数返回值与声明不符，不为空\n";
            return false;
        }
        return true;
    }
    else {
        auto ExprType = ExprMap.at(return_ptr->expr_ptr.value().get()).retType;
        auto [cast_op,msg] = ExprType.cast_to(&retType);
        if(cast_op == AST::CAST_OP::INVALID) {
            std::cerr<<std::format("return Expr，类型转换失败，源类型{}，无法转换成{}\n",
                toString_view(ExprType.format()),
                toString_view(retType.format())
            );
            return false;
        }
        else {
            if(!msg.empty()) {
                std::cerr<<"类型转换警告："<<toString_view(msg)<<std::endl;
            }
            ExprMap.at(return_ptr->expr_ptr.value().get()).castType = retType;
            ExprMap.at(return_ptr->expr_ptr.value().get()).cast_op = cast_op;
            return true;
        }
    }
}   


//主要工作：计算每个Expr返回类型，记录是否是左值 检查ArithExpr

class TypingCheckVisitor : public AST::ASTVisitor {
public:
    SemanticSymbolTable * rootTable;
    SemanticSymbolTable * currentTable;
    std::stack<AST::ASTNode *> ASTstack;
    std::stack<AST::FuncDeclare *> FuncStack;
    ExprTypeCheckMap ExprMap;
    bool gotERROR = false;
    std::u8string ERRORstr;
    bool build(SemanticSymbolTable * symtab , AST::AbstractSyntaxTree * tree) {
        ExprMap.clear();
        rootTable = symtab;
        currentTable = symtab;
        tree->root->accept(*this);
        if(gotERROR) {
            std::cerr<<"类型检查未通过\n";
        }
        return false;
    }
    
    inline void enter(AST::ASTNode * node) override{
        if(gotERROR) { return; }
        ASTstack.push(node);
        if(node->subType == AST::ASTSubType::FuncDeclare) {
            FuncStack.push(static_cast<AST::FuncDeclare *>(node) );
        }
        return;
    }
    inline void visit(AST::ASTNode * node) override {
        if(gotERROR) { return; }
        if(node->Ntype == AST::ASTType::Expr ) {
            parseExprCheck(static_cast<AST::Expr *>(node),ExprMap);
        }
        if(node->Ntype == AST::ASTType::Stmt) {
            parseStmtCheck(static_cast<AST::Stmt *>(node),ExprMap);
        }
        if(node->Ntype == AST::ASTType::ASTBool) {
            parseBoolCheck(static_cast<AST::ASTBool *>(node),ExprMap);
        }   
    }
    inline void quit(AST::ASTNode * node) override {
        if(ASTstack.empty()) {
            gotERROR = true;
            ERRORstr = u8"Visitor AST栈深度结构损坏";
            return;
        }
        ASTstack.pop();
        if(ASTstack.empty()) return;
        if(node->subType == AST::ASTSubType::Return) {
            parseReturnCheck(static_cast<AST::Return *>(node),rootTable,FuncStack.top(),ExprMap);
        }
        if(node->subType == AST::ASTSubType::FuncDeclare) {
            FuncStack.pop();
        }
        auto parent = ASTstack.top();
    }
};

class ASTContentVisitor : public AST::ASTVisitor {
int depth = 0;
public:
    bool moveSequence = false;

    inline void print(AST::ASTNode * node) {
    std::cout<<ASTSubTypeToString(node->subType);
        if(dynamic_cast<AST::ArithExpr *>(node)) {
            auto eptr = static_cast<AST::ArithExpr *>(node);
            if(eptr->Op == AST::BinaryOpEnum::ADD) {
                std::cout<<" +";
            } else if(eptr->Op == AST::BinaryOpEnum::MUL) {
                std::cout<<" *";
            }
        }
        if(dynamic_cast<AST::ConstExpr *>(node)) {
            std::cout<<" ";
            auto eptr = static_cast<AST::ConstExpr *>(node);
            auto v = eptr->value;
            auto visitors = overload {
                [](int v) {std::cout<<v;},
                [](float v) {std::cout<<v;},
                [](uint64_t v) {std::cout<<v;},
            };
            std::visit(visitors,v);
        }
        if(dynamic_cast<AST::SymIdNode *>(node)) {
            std::cout<<" ";
            auto eptr = static_cast<AST::SymIdNode *>(node);
            std::cout<<toString_view(eptr->Literal)<<" ";
            if(eptr->symEntryPtr) {
                std::cout<<static_cast<SymbolEntry*>(eptr->symEntryPtr)->symid<<" ";
            }
            
        }
        std::cout<<std::endl;
    }

    virtual void visit(AST::ASTNode* node) {
        if(moveSequence) {
            this->print(node); 
        }
        return;
    }
    inline virtual void enter(AST::ASTNode* node) {
        
        if(!moveSequence) {
            for(int i = 0 ;i <depth;i++) {
                std::cout<<" ";
            }
            this->print(node); 
        }
        depth++;
        return;
    }
    inline virtual void quit(AST::ASTNode* node) {
        depth--;
        return;
    }
};


inline void test_Semantic_main() {
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
    int a = 1;
    int b = 2;
    a = 10;
    b = 20;
    if (a < b) {
        return 1 ;
    }
    return 0;
}
    )";
    auto ss = Lexer::scan(toU8str(myprogram));
    for(int i= 0 ;i < ss.size() ; i++) {
        auto q = ss[i];
        std::cout<<"["<<toString(q.type)<<" "<<toString(q.value)<<" "<<i<<"]";
    }
    std::cout<<"\n";
    astT.BuildCommonAST(ss);
    //printCommonAST(astT.root);
    AST::mVisitor v;
    //astT.root->accept(v);
    std::cout<<std::endl;

    std::u8string myprogram2 = u8R"(
    int acc =1;
    float **** b;
    int main2(int c [][30] ;float b ) {
        int a = 5;
    }
    int main(int p;float L) {
        int a = 200 * 4.11111;
        int * refa = &a;
        *refa = 4.2;
        *(a + 1) = 5;
        int q = 4;
        if(4 <= 3) {
            int b = 8;
            b = 12 + 20;
        }
        {
            int q = 20 + 14;
            q = 20.2;
        }
        q = 4.2;
        a = 16 * 5 + 8.9;
        int b = 8;
        int c [10][30];
        c[0][0] = 0;
        main2 (c , 5.4);
    }
    float q = 4.2;
    
    )";
    auto ss2 = Lexer::scan(toU8str(myprogram2));
    for(int i= 0 ;i < ss2.size() ; i++) {
        auto q = ss2[i];
        std::cout<<"["<<toString(q.type)<<" "<<toString(q.value)<<" "<<i<<"]";
    }
    std::cout<<std::endl;
    astT.BuildSpecifiedAST(ss2);
    ASTContentVisitor v2;
    AST::ConstantFoldingVisitor v3;
    Semantic::SymbolTableBuildVisitor v4;
    Semantic::TypingCheckVisitor v5;
    SemanticSymbolTable tmp;
    //v2.moveSequence = true;
    astT.root->accept(v3);
    astT.root->accept(v2);
    
    v4.build(&tmp,&astT);
    tmp.printTable();
    tmp.checkTyping();
    astT.root->accept(v2);
    
    v5.build(&tmp,&astT);
    return;
    
}


}


#endif

