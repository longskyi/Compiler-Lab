#include "AST/AST.h"
#include "AST/NodeType/ASTBool.h"
#include "AST/NodeType/Stmt.h"
#include "stringUtil.h"

namespace AST
{

StateId startStateId(0);

bool AbstractSyntaxTree::BuildCommonAST(const std::vector<Lexer::scannerToken_t> & tokens) {
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
        auto Ids = symtab.find_index_type(token.type);
        if(Ids.size()!=1) {
            std::cerr<<"文法内部冲突, token type:"<< toString(token.type) <<" - symbolId解析错误";
            throw std::runtime_error("文法内部冲突, type 解析错误");
        }
        tokenSymId = Ids[0];
        
        action now_action;
        if(! actionTable[StateStack.top()].count(tokenSymId)) {
            for(auto p : states[StateStack.top()]) {
                auto prod_it = Productions.find(p.producId);
                if(prod_it == Productions.end()) {
                    std::cerr << "Invalid production ID in state";
                    continue;
                }
                const auto& prod = prod_it->second;
                std::cout << toString(formatProduction(prod, p.dot_pos, symtab)) << " ";
            }
            std::cout<<std::endl;
            
            std::cerr << "unexpected token :" << toString(symtab[tokenSymId].sym()) << " atPos: " << token_i << "\n";
            std::vector<std::u8string> expectedSym;
            for(const auto & s : actionTable[StateStack.top()]) {
                if(symtab[s.first].is_terminal()) {
                    expectedSym.push_back(symtab[s.first].sym());
                }
            }
            std::cerr << " Expected token:[";
            for(const auto & s : expectedSym ) {
                std::cerr <<"\"" <<toString(s) <<"\",";
            }
            std::cerr <<"]"<<std::endl;
            return false;
        }
    
        now_action = actionTable[StateStack.top()].at(tokenSymId);
    
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
            if(! Productions.count(std::get<ProductionId>(now_action))) {
                std::cerr << "unable to find Production with id :" <<std::get<ProductionId>(now_action);
                return false;
            }
            auto prod = Productions.at(std::get<ProductionId>(now_action));
            if(symtab[prod.lhs()].sym() == u8"START") {
                root = std::move(symStack.top());
                symStack.pop();
                return true;
            }
            size_t popsize = prod.rhs().size();
            ASTCommonNode nt;
            nt.type = symtab[prod.lhs()].sym();
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
            if(! gotoTable[StateStack.top()].count(SymbolId(prod.lhs()) )) {
                std::cerr << "AST构建内部错误 , goto表找不到对应非终结符转移规则" ;
                return false;
            }
            auto nextState = gotoTable[StateStack.top()].at(SymbolId(prod.lhs()));
            StateStack.push(nextState);
            //不移进
        }
    }
    std::unreachable();
    // root = std::move(symStack.top());
    // symStack.pop();
    // return true;
}

bool AbstractSyntaxTree::BuildSpecifiedAST(const std::vector<Lexer::scannerToken_t> & tokens) {
    //Not fully implement
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
        auto Ids = symtab.find_index_type(token.type);
        if(Ids.size()!=1) {
            std::cerr<<"文法内部冲突, token type:"<< toString(token.type) <<" - symbolId解析错误";
            throw std::runtime_error("文法内部冲突, type 解析错误");
        }
        tokenSymId = Ids[0];
        
        action now_action;
        if(! actionTable[StateStack.top()].count(tokenSymId)) {
            for(auto p : states[StateStack.top()]) {
                auto prod_it = Productions.find(p.producId);
                if(prod_it == Productions.end()) {
                    std::cerr << "Invalid production ID in state";
                    continue;
                }
                const auto& prod = prod_it->second;
                std::cout << toString(formatProduction(prod, p.dot_pos, symtab)) << " ";
            }
            std::cout<<std::endl;
            
            std::cerr << "unexpected token :" << toString(symtab[tokenSymId].sym()) << " atPos: " << token_i << "\n";
            std::vector<std::u8string> expectedSym;
            for(const auto & s : actionTable[StateStack.top()]) {
                if(symtab[s.first].is_terminal()) {
                    expectedSym.push_back(symtab[s.first].sym());
                }
            }
            std::cerr << " Expected token:[";
            for(const auto & s : expectedSym ) {
                std::cerr <<"\"" <<toString(s) <<"\",";
            }
            std::cerr <<"]"<<std::endl;
            return false;
        }
    
        now_action = actionTable[StateStack.top()].at(tokenSymId);
    
        auto action_Visitor = overload {
            [](StateId v) -> bool {return true;},
            [](ProductionId v) -> bool {return false;}
        };
        bool isShift = std::visit(action_Visitor,now_action);
        if(isShift) {
            //移进
            StateStack.push( std::get<StateId>(now_action) );
            //给token建结点
            TermSymNode nt;
            nt.token_type = token.type;
            nt.value = token.value;
            symStack.emplace(std::make_unique<TermSymNode>(std::move(nt)));
            //移进
            token_i++;
        }
        else {
            //归约
            if(! Productions.count(std::get<ProductionId>(now_action))) {
                std::cerr << "unable to find Production with id :" <<std::get<ProductionId>(now_action);
                return false;
            }
            auto prod = Productions.at(std::get<ProductionId>(now_action));
            if(symtab[prod.lhs()].sym() == u8"START") {
                root = std::move(symStack.top());
                symStack.pop();
                return true;
            }
            size_t popsize = prod.rhs().size();
            auto nt = std::make_unique<AST::NonTermProdNode>();
            nt->prodId = prod.index();
            std::vector<unique_ptr<ASTNode>> ntchilds;
            for(int i=0;i<popsize;i++) {
                ntchilds.emplace_back(std::move(symStack.top()));
                StateStack.pop();
                symStack.pop();
            }
            for(int i = popsize-1;i>=0;i--) {
                nt->childs.emplace_back(std::move(ntchilds[i]));
            }
            unique_ptr<ASTNode> specifiedNode = AST_specified_node_construct(std::move(nt),this);
            if(!specifiedNode) {
                throw std::runtime_error("");
            }
            symStack.emplace(std::move(specifiedNode));
            //查表当前归约非终结符后的下一个状态
            if(! gotoTable[StateStack.top()].count(SymbolId(prod.lhs()) )) {
                std::cerr << "AST构建内部错误 , goto表找不到对应非终结符转移规则" ;
                return false;
            }
            auto nextState = gotoTable[StateStack.top()].at(SymbolId(prod.lhs()));
            StateStack.push(nextState);
            //不移进
        }
    }
    std::unreachable();
    // root = std::move(symStack.top());
    // symStack.pop();
    // return true;

}

void printCommonAST(const unique_ptr<ASTNode>& node, int depth) {
    // Check if node is ASTCommonNode using dynamic_cast
    if (auto commonNode = dynamic_cast<ASTCommonNode*>(node.get())) {
        // Print indentation based on depth
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        
        // Print node type and value
        std::cout << "Type: " << std::string(commonNode->type.begin(), commonNode->type.end())
                  << ", Value: " << std::string(commonNode->value.begin(), commonNode->value.end())
                  << std::endl;
        
        // Recursively print children with increased depth
        for (const auto& child : commonNode->childs) {
            printCommonAST(child, depth + 1);
        }
    } else {
        // Print message for non-common nodes
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        std::cout << "非通用结点，跳过" << std::endl;
    }
}

unique_ptr<ASTNode> AST_specified_node_construct(unique_ptr<NonTermProdNode> prodNode , AbstractSyntaxTree * ast_tree) {
    unique_ptr<ASTNode> ret;
    ret = Program::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; } 
    ret = BlockItem::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = BlockItemList::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = IdDeclare::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = ConstExpr::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = pType::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = ArgList::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Arg::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Block::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = FuncDeclare::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = IdValueExpr::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = DerefExpr::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = CallExpr::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = ArithExpr::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Param::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = ParamList::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Arg::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = ArgList::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = BlockStmt::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Assign::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Branch::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Loop::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = FunctionCall::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Return::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = ASTBool::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Expr::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = pTypeList::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    ret = Dimensions::try_constructS(prodNode.get(),ast_tree);
    if(ret) { return ret; }
    std::cerr <<"构造失败"<< toString_view(LCMPFileIO::formatProduction(ast_tree->Productions.at(prodNode->prodId),ast_tree->symtab));
    
    return nullptr;
}

void AST_test_main() {
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
    mVisitor v;
    //astT.root->accept(v);
    std::cout<<std::endl;

    std::u8string myprogram2 = u8R"(
    int acc =1;
    float b =2;
    int main() {
        int a = 200 * 4.11111;
        a = 16 * 5 + 8;
        int b = 8;
        int c [10];
    }
    )";
    auto ss2 = Lexer::scan(toU8str(myprogram2));
    for(int i= 0 ;i < ss2.size() ; i++) {
        auto q = ss2[i];
        std::cout<<"["<<toString(q.type)<<" "<<toString(q.value)<<" "<<i<<"]";
    }
    std::cout<<std::endl;
    astT.BuildSpecifiedAST(ss2);
    ASTEnumTypeVisitor v2;
    ConstantFoldingVisitor v3;
    //v2.moveSequence = true;
    astT.root->accept(v3);
    astT.root->accept(v2);
    return;
    
}

} // namespace AST


