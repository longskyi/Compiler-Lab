#include<iostream>
#include<string>
#include<map>
#include<unordered_map>
#include<filesystem>
#include"stringUtil.h"
#include"SyntaxType.h"
#include"grammarRead.h"
namespace fs = std::filesystem;

int main()
{
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
    }
    

    //string_test_main();
    std::u8string u8str = u8"你好世界";

    return 0;
}