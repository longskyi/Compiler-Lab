#include<iostream>
#include<string>
#include<map>
#include<unordered_map>
#include<filesystem>
#include"stringUtil.h"
#include"SyntaxType.h"
#include"fileIO.h"
#include"DFA.h"
#include"lexer.h"
#include"parserGen.h"
namespace fs = std::filesystem;

int main()
{
    //NFA_NS::test_regex_to_dfa();
    std::string myprogram = R"(
    int 原始(int 这是x;) {
        这是y=这是x+5;
        return 这是y};
    void 嗨嗨嗨，你在干什么(int y;) {
        int 𰻞;
        void bar(int x; int soo();) {
        if(x>3) bar(x/3,soo(),) else 𰻞 = soo(x);
        print 𰻞};
        bar(y,raw(),)};
    foo(6,)
    )";
    auto ss = Lexer::scan(toU8str(myprogram));
    for(auto q : ss) {
        std::cout<<"["<<toString(q.type)<<","<<toString(q.value)<<"]";
    }
    return 0;
    DFA_test_main();
    parserGen_test_main();
    return 0;
    fs::path curr_path = fs::current_path();
    fs::path gpath = curr_path /".."/ u8"grammar" / u8"grammar.txt";
    fs::path tpath = curr_path /".."/ u8"grammar" / u8"terminal.txt";

    SymbolTable symtab;
    std::vector<Production> productions;
    try {
        readTerminals(symtab,tpath);
        readProductionRule(symtab,productions,gpath);
    }
    catch(const std::exception& e) {
        std::cerr << "read grammar failed: " ;
        std::cerr << e.what() << '\n';
        return 0;
    }

    auto rs = symtab.symbols();
    for(auto c : rs) {
        std::cout<<toString(c.sym())<<" "<<toString(c.pattern())<<" "<<std::endl;
    }
    for(auto q : productions) {
        std::cout<<toString(symtab[q.lhs()].sym()) <<" ";
        for(auto b : q.rhs()) {
            std::cout<<toString(symtab[b].sym()) <<" ";
        }
        std::cout<<std::endl;
    }
    

    //string_test_main();
    std::u8string u8str = u8"你好世界";

    return 0;
}