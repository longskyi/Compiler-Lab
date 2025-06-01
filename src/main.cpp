#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <fstream>
#include <optional>
#include <charconv>
#include <format>
#include <filesystem>
#include <algorithm>
#include <variant>
#include "parserGen.h"
#include "asmGen.h"
#include "AST/AST.h"
namespace fs = std::filesystem;

struct SLRParams {
    fs::path grammar_file;
    fs::path terminal_file;
    std::optional<fs::path> conflict_file;
    std::string output_name;
};

struct ASTParams {
    fs::path input_file;
    std::optional<fs::path> param_file;
    std::string output_name;
};

struct SParams {
    fs::path input_file;
    std::string output_name;
};

using CommandParams = std::variant<SLRParams, ASTParams, SParams>;

void print_help() {
    std::cout << R"(
LCCompiler
用法:
  1. SLR语法分析:
    LCCompiler -slr <grammar文件> <terminal文件> [<SLR冲突解决文件>] -o=<输出文件名>

  2. 生成AST,若使用外置参数文件则生成通用AST结点:
    LCCompiler -ast -i=<输入文件> [-p=<参数文件>] -o=<输出文件名>

  3. 生成汇编:
    LCCompiler -s -i=<输入文件> -o=<输出文件名>

  4. 帮助:
    LCCompiler -h 或 LCCompiler --help

选项说明:
  -slr     执行SLR语法分析
  -ast     执行AST处理
  -s       执行S处理
  -i       指定输入文件
  -p       指定参数文件(可选)
  -o       指定输出文件名(不含后缀)
  -h/--help 显示帮助信息
)" << std::endl;
}

std::optional<CommandParams> parse_args(int argc, const char* argv[]) {
    if (argc < 2) {
        print_help();
        return std::nullopt;
    }

    std::string_view command(argv[1]);

    if (command == "-h" || command == "--help") {
        print_help();
        return std::nullopt;
    }

    std::vector<std::string_view> args(argv + 2, argv + argc);

    if (command == "-slr") {
        if (args.size() < 3) {
            std::cerr << "错误: SLR命令需要至少3个参数\n";
            return std::nullopt;
        }

        SLRParams params;
        size_t pos = 0;

        // 解析前两个必需文件参数
        params.grammar_file = args[pos++];
        params.terminal_file = args[pos++];

        // 检查第三个可选文件参数
        if (pos < args.size() && !args[pos].starts_with("-o=")) {
            params.conflict_file = args[pos++];
        }

        // 解析输出参数
        bool output_found = false;
        for (; pos < args.size(); ++pos) {
            if (args[pos].starts_with("-o=")) {
                params.output_name = args[pos].substr(3);
                output_found = true;
                break;
            }
        }

        if (!output_found) {
            std::cerr << "错误: 缺少输出文件名(-o参数)\n";
            return std::nullopt;
        }

        return params;
    }
    else if (command == "-ast") {
        ASTParams params;
        bool input_found = false;
        bool output_found = false;

        for (const auto& arg : args) {
            if (arg.starts_with("-i=")) {
                params.input_file = arg.substr(3);
                input_found = true;
            }
            else if (arg.starts_with("-p=")) {
                params.param_file = arg.substr(3);
            }
            else if (arg.starts_with("-o=")) {
                params.output_name = arg.substr(3);
                output_found = true;
            }
        }

        if (!input_found || !output_found) {
            std::cerr << "错误: AST命令需要-i和-o参数\n";
            return std::nullopt;
        }

        return params;
    }
    else if (command == "-s") {
        SParams params;
        bool input_found = false;
        bool output_found = false;

        for (const auto& arg : args) {
            if (arg.starts_with("-i=")) {
                params.input_file = arg.substr(3);
                input_found = true;
            }
            else if (arg.starts_with("-o=")) {
                params.output_name = arg.substr(3);
                output_found = true;
            }
        }

        if (!input_found || !output_found) {
            std::cerr << "错误: S命令需要-i和-o参数\n";
            return std::nullopt;
        }

        return params;
    }
    else {
        std::cerr << "错误: 未知命令 " << command << "\n";
        print_help();
        return std::nullopt;
    }
}

void process_slr(const SLRParams& params) {
    std::cout << std::format(
        "SLR配置生成:\n"
        "语法文件: {}\n"
        "终结符文件: {}\n",
        params.grammar_file.string(),
        params.terminal_file.string()
    );

    if (params.conflict_file) {
        std::cout << "SLR冲突解决文件: " << params.conflict_file->string() << "\n";
    }
    auto start = std::chrono::high_resolution_clock::now();

    
    auto astbase = parserGen(
        params.grammar_file,
        params.terminal_file,
        params.conflict_file
    );

    std::string output_filename = params.output_name + ".json";
    std::cout << "输出文件: " << output_filename << "\n";

    std::ofstream o(output_filename);
    nlohmann::json j = astbase;
    o << std::setw(4) << j;  // std::setw(4) 控制缩进
    o.close();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Program took " 
              << duration.count() << " microseconds (" 
              << duration.count() / 1000.0 << " milliseconds) to execute.\n";
    
}

void process_ast(const ASTParams& params) {
    std::cout << std::format(
        "处理AST转换:\n"
        "输入文件: {}\n",
        params.input_file.string()
    );

    if (params.param_file) {
        std::cout << "参数文件: " << params.param_file->string() << "\n";
    }

    nlohmann::json j2;
    if (params.param_file) {
        std::ifstream file(params.param_file.value());
        file >> j2;  // 从文件流解析
    }
    else {
        std::ifstream file("defaultGrammar.json");
        file >> j2;  // 从文件流解析
    }
    ASTbaseContent astbase2 = j2;
    AST::AbstractSyntaxTree astT(astbase2);    
    std::string program;
    program = readFileToString(params.input_file);
    auto ss = Lexer::scan(toU8str(program));
    if (params.param_file) {
        astT.BuildCommonAST(ss);
        AST::mVisitor v;
        astT.root->accept(v);
    }
    else {
        std::ofstream ofile(params.output_name+".ast");
        astT.BuildSpecifiedAST(ss);
        AST::ASTEnumTypeVisitor v2(ofile);
        astT.root->accept(v2);
    }
    return;
}

void process_s(const SParams& params) {
    
    auto out = params.output_name;

    auto start = std::chrono::high_resolution_clock::now();

    std::ifstream file("defaultGrammar.json");
    nlohmann::json j2;
    file >> j2;  // 从文件流解析
    ASTbaseContent astbase2 = j2;
    AST::AbstractSyntaxTree astT(astbase2);
    
    std::string program;
    std::ifstream file2(params.input_file);
    if(!file2.is_open()) {
        std::cout<<"无法打开文件："<<params.input_file<<std::endl;
        return;
    }
    program = readFileToString(params.input_file);

    std::ofstream tokenFile(params.output_name+".token");
    std::ofstream ASTFile(params.output_name+".astFile");
    std::ofstream IRFile(params.output_name+".ir");
    std::ofstream ASMFile(params.output_name+".s");
    std::ofstream SymFile(params.output_name+".symTable");

    auto ss = Lexer::scan(toU8str(program));
    for(int i= 0 ;i < ss.size() ; i++) {
        auto q = ss[i];
        tokenFile<<"["<<toString(q.type)<<" "<<toString(q.value)<<" "<<i<<"]";
        if(q.type == u8"ERR") {
            std::cerr<<std::format("Lexer解析失败:无法解析的符号{}\n",toString_view(q.value));
            return;
        }
    }
    if(!astT.BuildSpecifiedAST(ss)) {
        std::cerr<<"AST生成失败\n"<<std::endl;
        return;
    }

    Semantic::ASTContentVisitor v2(ASTFile);
    AST::ConstantFoldingVisitor v3;
    Semantic::SymbolTableBuildVisitor v4;
    Semantic::TypingCheckVisitor v5;
    Semantic::SemanticSymbolTable symtab;
    //  AST构建
    astT.root->accept(v3);
    //  打印AST
    astT.root->accept(v2);
    
    if(!v4.build(&symtab,&astT)) {
        std::cout<<"符号表生成错误\n";
        return;
    }
    if(!symtab.checkTyping()) {
        std::cout<<"符号表类型检查失败\n";
        return;
    }
    astT.root->accept(v2);
    Semantic::ExprTypeCheckMap ExprMap;
    if(!v5.build(&symtab,&astT,&ExprMap)) {
        std::cout<<"类型检查未通过\n";
        return;
    }
    symtab.arrangeMem();
    symtab.printTable(0,SymFile);
    IR::IRContent irbase;
    IR::EntryOperandMap entry_map;
    IR::IRGenVisitor v6;
    if(!v6.build(&symtab,&astT,&irbase,&ExprMap,&entry_map)) {
        std::cout<<"IR生成失败\n";
        return;
    }
    irbase.print(IRFile);
    ASM::X64ASMGenerator g1;
    if(!g1.genASM(irbase,ASMFile)) {
        std::cout<<"ASM生成失败\n";
        return;
    }

    std::cout<<"运行成功\n";
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Function took " 
              << duration.count() << " microseconds (" 
              << duration.count() / 1000.0 << " milliseconds) to execute.\n";
    
}

int main(int argc, const char* argv[]) {

    int argcT = 4;
    const char * testargv [] ={"","-s","-i=testCase/24.src","-o=test"};
    
    auto params = parse_args(argc, argv);
    //auto params = parse_args(argcT, testargv);
    if (!params) {
        return 1;
    }
    try {
        std::visit([](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, SLRParams>) {
                process_slr(arg);
            }
            else if constexpr (std::is_same_v<T, ASTParams>) {
                process_ast(arg);
            }
            else if constexpr (std::is_same_v<T, SParams>) {
                process_s(arg);
            }
        }, *params);
    }
    catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << "\n";
        return 1;
    }

    return 0;
}