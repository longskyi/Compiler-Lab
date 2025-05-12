#include<iostream>
#include<string>
#include<map>
#include<unordered_map>
#include<filesystem>
#include"stringUtil.h"
#include"SyntaxType.h"
#include"grammarRead.h"
#include"parserGen.h"
namespace fs = std::filesystem;

int main()
{
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

    auto r = symtab.symbols();
    for(auto c : r) {
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