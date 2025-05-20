#ifndef M_LCMP_SEMANTIC_HEADER
#define M_LCMP_SEMANTIC_HEADER

//正常编译时的符号表
#include <cstdint>
#include <memory>
#include <vector>
#include "AST/NodeType/ASTbaseType.h"

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
    uint64_t offset = 0;
    symEntryID symid;
    SymbolEntry();  //自动递增id
    unique_ptr<SemanticSymbolTable> subTable;
};

class SemanticSymbolTable {
public:
    std::u8string tabletag; //this is not a key;
    SemanticSymbolTable * outer = nullptr;
    int64_t width = -1;
    std::vector<unique_ptr<SymbolEntry>> entries;
    inline SymbolEntry * lookup(const std::u8string & name) {
        for(const auto & sym : entries) {
            if(sym->symbolName == name) {
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

};

inline SymbolEntry::SymbolEntry() : symid(symEntryID(next_index++)) {}


//构造符号表，并绑定（存储到AST节点上）
class SymBolTableBuildVisitor : public AST::ASTVisitor {
public:
    SemanticSymbolTable * rootTable;
    SemanticSymbolTable * currentTable;
    AST::AbstractSyntaxTree * AStree;
    bool gotError = false;
    bool build(SemanticSymbolTable * root , AST::AbstractSyntaxTree * tree) {
        if(root == nullptr) {
            std::cerr<<"符号表生成输入表为空指针";
            return false;
        }
        if(tree == nullptr) {
            std::cerr<<"符号表生成输入树为空指针";
            return false;
        }
        rootTable = root;
        currentTable = root;
        AStree = tree;

        return gotError;
    }


    //需要关注 block
    void enter(AST::ASTNode* node) {
        if(gotError) return;
    }
    void visit(AST::ASTNode* node) {
        if(gotError) return;
    }
    void quit(AST::ASTNode* node) {
        if(gotError) return;
    }
};

}


#endif

