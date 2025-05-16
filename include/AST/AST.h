#ifndef LCMP_AST_HEADER
#define LCMP_AST_HEADER
#include <memory>
#include <string>
#include <stack>
#include <stdexcept>
#include "templateUtil.h"
#include "SyntaxType.h"
#include "lexer.h"

namespace AST
{

using u8string = std::u8string;

using std::unique_ptr;

StateId startStateId(0);

enum class ASTType {
    Common
};

enum class ASTSubType {
    SubCommon
};
class ASTNode
{
public:
    ASTType Ntype;
    ASTSubType subType;
    ASTNode() {
        Ntype = ASTType::Common;
        subType = ASTSubType::SubCommon;
    }
    virtual ~ASTNode() = default;
};

class ASTCommonNode : public ASTNode
{
public:
    bool isLeaf() {
        return childs.size() == 0;
    }
    u8string type; //原始类型-非终结符
    u8string value;
    std::vector<unique_ptr<ASTNode>> childs;
};

typedef struct ASTbaseContent {
    std::vector<std::vector<dotProdc>> states;
    std::vector<std::unordered_map<SymbolId, StateId>> gotoTable;
    std::vector<std::unordered_map<SymbolId, action>> actionTable;
    SymbolTable symtab;
    std::unordered_map<ProductionId, Production> Productions;
}ASTbaseContent;

class AbstractSyntaxTree {
public:
    unique_ptr<ASTNode> root;
    const std::vector<std::vector<dotProdc>> states;
    const std::vector<std::unordered_map<SymbolId, StateId>> gotoTable;
    const std::vector<std::unordered_map<SymbolId, action>> actionTable;
    const SymbolTable symtab;
    const std::unordered_map<ProductionId, Production> Productions;
    AbstractSyntaxTree(ASTbaseContent inp)
     : states(std::move(inp.states)),gotoTable(std::move(inp.gotoTable)), actionTable(std::move(inp.actionTable)), symtab(std::move(inp.symtab)), Productions(std::move(inp.Productions))
    {
        root = nullptr;
    }
    bool BuildCommonAST(const std::vector<Lexer::scannerToken_t> & tokens) {
        if(tokens.back().value != u8"$") {
            std::cerr<<"tokens末尾非结束符";
            return false;
        }
        root = nullptr;
        std::stack<StateId> StateStack;
        std::stack<unique_ptr<ASTNode>> symStack;
        // auto curr_state = startStateId;
        StateStack.push(startStateId);
        for(int token_i = 0 ; token_i < tokens.size() ;){
            auto token = tokens[token_i];
            SymbolId tokenSymId;
            //获取token对应symbolId
            if(token.type == u8"ID" || token.type == u8"NUM" || token.type == u8"FLO") {
                auto Ids = symtab.find_index_type(token.type);
                if(Ids.size()!=1) {
                    throw std::runtime_error("文法内部错误，ID NUM FLO 解析错误");
                }
                tokenSymId = Ids[0];
            }
            else {

                auto opn = symtab.find_index(token.value);
                if(opn) {
                    tokenSymId = opn.value();
                }
                else {
                    std::cerr<<"构建通用AST失败";
                    return false;
                }
            }
            action now_action = actionTable[StateStack.top()].at(tokenSymId);

            auto action_Visitor = overload {
                [](StateId v) -> bool {return true;},
                [](ProductionId v) -> bool {return false;}
            };
            bool isShift = std::visit(action_Visitor,now_action);
            if(isShift) {
                //移进
                StateStack.push( std::get<StateId>(now_action) );
                //给token建结点
                ASTCommonNode nt;
                nt.type = token.type;
                nt.value = token.value;
                symStack.emplace(std::make_unique<ASTCommonNode>(std::move(nt)));
                //移进
                token_i++;
            }
            else {
                //归约
                auto prod = Productions.at(std::get<ProductionId>(now_action));
                size_t popsize = prod.rhs().size();
                ASTCommonNode nt;
                nt.type = symtab[prod.lhs()].tokentype();
                nt.value = u8"";
                std::vector<unique_ptr<ASTNode>> ntchilds;
                for(int i=0;i<popsize;i++) {
                    ntchilds.emplace_back(std::move(symStack.top()));
                    StateStack.pop();
                    symStack.pop();
                }
                for(int i = popsize-1;i>=0;i--) {
                    nt.childs.emplace_back(std::move(ntchilds[i]));
                }
                symStack.emplace(std::make_unique<ASTCommonNode>(std::move(nt)));
                //查表当前归约非终结符后的下一个状态
                auto nextState = gotoTable[StateStack.top()].at(SymbolId(prod.lhs()));
                StateStack.push(nextState);
                //不移进
            }
        }
        root = std::move(symStack.top());
        symStack.pop();
    }
    virtual ~AbstractSyntaxTree() = default;
};
    






void test_main() {
    
}

} // namespace AST


#endif