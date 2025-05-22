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


bool parseExprCheck(AST::Expr * mainExpr_ptr , ExprTypeCheckMap & ExprMap);

//不包含return检查
bool parseStmtCheck(AST::Stmt * mainStmt_ptr, ExprTypeCheckMap & ExprMap);

bool parseBoolCheck(AST::ASTBool * ASTbool_ptr , ExprTypeCheckMap & ExprMap);

bool parseReturnCheck(AST::Return * return_ptr , SemanticSymbolTable * rootTable ,AST::FuncDeclare * func_ptr, ExprTypeCheckMap & ExprMap);


//主要工作：计算每个Expr返回类型，记录是否是左值 检查ArithExpr

class TypingCheckVisitor : public AST::ASTVisitor {
public:
    SemanticSymbolTable * rootTable;
    SemanticSymbolTable * currentTable;
    std::stack<AST::ASTNode *> ASTstack;
    std::stack<AST::FuncDeclare *> FuncStack;
    ExprTypeCheckMap * ExprMap;
    bool gotERROR = false;
    std::u8string ERRORstr;
    bool build(SemanticSymbolTable * symtab , AST::AbstractSyntaxTree * tree ,ExprTypeCheckMap * ExprMap_ ) {
        ExprMap = ExprMap_;
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
            gotERROR |= ! parseExprCheck(static_cast<AST::Expr *>(node),*ExprMap);
        }
        if(node->Ntype == AST::ASTType::Stmt) {
            gotERROR |= ! parseStmtCheck(static_cast<AST::Stmt *>(node),*ExprMap);
        }
        if(node->Ntype == AST::ASTType::ASTBool) {
            gotERROR |= ! parseBoolCheck(static_cast<AST::ASTBool *>(node),*ExprMap);
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
            gotERROR |= ! parseReturnCheck(static_cast<AST::Return *>(node),rootTable,FuncStack.top(),*ExprMap);
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
    ExprTypeCheckMap ExprMap;
    v5.build(&tmp,&astT,&ExprMap);
    return;
    
}


}


#endif

