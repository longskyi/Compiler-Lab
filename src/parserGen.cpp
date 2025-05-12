#include"SyntaxType.h"
#include"stringUtil.h"
#include"grammarRead.h"
#include<variant>
#include<cassert>
#include<iostream>
#include<unordered_set>
#include<filesystem>

const std::u8string epsilon = u8"ε"; // 显式定义ε


std::unordered_map<NonTerminalId, std::unordered_set<std::u8string>> computeFirst(
    const SymbolTable& symtab,
    const std::unordered_map<ProductionId, Production>& Productions) 
{
    std::unordered_map<NonTerminalId, std::unordered_set<std::u8string>> FIRST;
    
    // 初始化所有非终结符的FIRST集
    for (auto ntid : symtab.nonTerminals()) {
        FIRST[ntid] = {};
    }
    bool changed;
    size_t loop_count = 0;
    
    do {
        changed = false;
        for (const auto & [_, Prod] : Productions) {
            auto ntid = NonTerminalId(Prod.lhs());
            bool all_epsilon = true;
            
            for (const auto& rhsid : Prod.rhs()) {
                const auto& sym = symtab[rhsid];
                
                if (sym.is_terminal()) {
                    // 终结符直接加入FIRST集
                    if (FIRST[ntid].insert(sym.sym()).second) {
                        changed = true;
                    }
                    all_epsilon = false;
                    break;
                } else {
                    // 非终结符的情况
                    auto rhs_ntid = NonTerminalId(rhsid);
                    // 避免直接左递归
                    if (rhs_ntid == ntid) {
                        if (FIRST[rhs_ntid].empty()) {
                            all_epsilon = false;    
                            break;
                        }
                        if (FIRST[rhs_ntid].count(epsilon) != 0) {
                            continue;
                        }
                        else {
                            all_epsilon = false;
                            break;
                        }  
                    }
                    // 添加非ε的符号
                    for (const auto& h : FIRST[rhs_ntid]) {
                        if (h != epsilon && FIRST[ntid].insert(h).second) {
                            changed = true;
                        }
                    }
                    // 如果不能推导出ε，就停止
                    if (FIRST[rhs_ntid].count(epsilon) == 0) {
                        all_epsilon = false;
                        break;
                    }
                }
            }
            // 如果所有符号都能推导出ε，则加入ε
            if (all_epsilon && FIRST[ntid].insert(epsilon).second) {
                changed = true;
            }
        }

        if (++loop_count > 50) {
            std::cerr << "FIRST computation failed to converge after 50 iterations\n";
            std::unreachable();
        }
    } while (changed);
    
    return FIRST;
}

std::unordered_set<std::u8string> computeFirstOfSequence(
    const SymbolTable& symtab,
    const std::unordered_map<NonTerminalId, std::unordered_set<std::u8string>>& FIRST,
    const std::vector<SymbolId>& sequence)
{
    std::unordered_set<std::u8string> result;
    bool all_epsilon = true;
    
    for (const auto& symid : sequence) {
        const auto& sym = symtab[symid];
        if (sym.is_terminal()) {
            result.insert(sym.sym());
            all_epsilon = false;
            break;
        } else {
            auto ntid = NonTerminalId(symid);
            for (auto h : FIRST.at(ntid)) {
                if (h != epsilon) result.insert(h);
            }
            if (FIRST.at(ntid).count(epsilon) == 0) {
                all_epsilon = false;
                break;
            }
        }
    }
    if (all_epsilon) {
        result.insert(epsilon);
    }
    return result;
}

std::unordered_map<NonTerminalId, std::unordered_set<std::u8string>> computeFollow(
    const SymbolTable& symtab,
    const std::unordered_map<ProductionId, Production>& Productions,
    const std::unordered_map<NonTerminalId, std::unordered_set<std::u8string>>& FIRST,
    NonTerminalId startSymbol)
{
    std::unordered_map<NonTerminalId, std::unordered_set<std::u8string>> FOLLOW;
    const std::u8string endmarker = u8"$";
    
    // 初始化
    for (auto ntid : symtab.nonTerminals()) {
        FOLLOW[ntid] = {};
    }
    FOLLOW[startSymbol].insert(endmarker);

    bool changed;
    size_t loop_count = 0;
    
    do {
        changed = false;
        for (const auto& [_, Prod] : Productions) {
            auto A = NonTerminalId(Prod.lhs());
            const auto& rhs = Prod.rhs();
            
            for (size_t i = 0; i < rhs.size(); ++i) {
                if (!symtab[rhs[i]].is_terminal()) {
                    auto B = NonTerminalId(rhs[i]);
                    auto beta = std::vector(rhs.begin() + i + 1, rhs.end());
                    std::vector<SymbolId> beta2;
                    beta2.reserve(beta.size()); 
                    for(auto id_ : beta) {
                        beta2.push_back(SymbolId(id_));
                    }
                    auto firstBeta = computeFirstOfSequence(symtab, FIRST, beta2);
                    
                    // 规则1
                    for (auto b : firstBeta) {
                        if (b != epsilon && FOLLOW[B].insert(b).second) {
                            changed = true;
                        }
                    }
                    
                    // 规则2
                    if (firstBeta.count(epsilon) || beta.empty()) {
                        for (auto f : FOLLOW[A]) {
                            if (FOLLOW[B].insert(f).second) {
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
        
        if (++loop_count > 50) {
            std::cerr << "FOLLOW computation failed to converge\n";
            std::unreachable();
        }
    } while (changed);
    
    return FOLLOW;
}

/**
 * @brief 原地构造闭包
 * @attention 对于epsilon产生式，其prod的rhs size 为 0
 */
void generateClosure(const SymbolTable & symtab,const std::unordered_map<ProductionId,Production> & Productions , std::vector<dotProdc> & state) {
    auto inState = [&state](const dotProdc & i) -> bool {
        for(auto & p : state ) {
            if(p.dot_pos == i.dot_pos && p.producId == i.producId) {
                return true;
            }
        }
        return false;
    };
    for(size_t i = 0 ; i < state.size() ; i++) {
        auto prod_it = Productions.find(state[i].producId);
        if(prod_it == Productions.end()) {
            std::cerr<<"ERROR PRODID FROM CLOSURE";
            std::unreachable();
        }
        const auto & prod = (*prod_it).second;
        if(prod.rhs().size() < state[i].dot_pos) {
            std::cerr<<"ERROR DOT POS";
            std::unreachable();
        }
        if(prod.rhs().size() == state[i].dot_pos) {
            //到达末尾，不增加闭包
            //含epsilon产生式
            continue;
        }
        const auto & sym = symtab[prod.rhs()[state[i].dot_pos]];
        if( sym.is_terminal()) {
            continue;
        }
        else {
            for(const auto & [pid,prodo] : Productions ) {
                dotProdc dpi = {0,pid};
                if(!inState(dpi)) {
                    state.push_back(dpi);
                }
            }
        }
    }
}

/**
 * @brief 新状态生成
 */
std::vector<dotProdc> generateState(const SymbolTable & symtab ,const std::unordered_map<ProductionId,Production> & Productions,const std::vector<dotProdc> &curr_state, const SymbolId symbolId) {
    std::vector<dotProdc> newState;
    for (auto & item : curr_state) {
        auto prod_it = Productions.find(item.producId);
        const auto & prod = (*prod_it).second;
        if(prod.rhs().size() == item.dot_pos) continue;
        if(prod.rhs()[item.dot_pos] != symbolId) continue;
        newState.emplace_back(dotProdc{ item.dot_pos + 1 , prod.index() });
    }
    generateClosure(symtab,Productions,newState);

    return newState;
}

void parserGen_test_main() {
    namespace fs = std::filesystem;
    std::u8string char1;
    //assert(u8len(char1)==1 && "u8char len not equal 1 fault");
    fs::path curr_path = fs::current_path();
    fs::path gpath = curr_path /".."/ u8"grammar" / u8"grammar.txt";
    fs::path tpath = curr_path /".."/ u8"grammar" / u8"terminal.txt";

    std::unordered_map<ProductionId,Production> Productions;
    std::vector<std::vector<dotProdc>> states; 
    std::vector<std::unordered_map<std::u8string,action>> actionTable; 
    std::vector<std::unordered_map<NonTerminalId,StateId>> gotoTable;
    SymbolTable symtab;

    std::vector<Production> mproductions;
    try {
        readTerminals(symtab,tpath);
        readProductionRule(symtab,mproductions,gpath);
    }
    catch(const std::exception& e) {
        std::cerr << "read grammar failed: " ;
        std::cerr << e.what() << '\n';
    }
    for(auto & prod : mproductions) {
        Productions.insert({ProductionId(prod.index()),prod});
    }

    auto FIRST = computeFirst(symtab,Productions);
    auto FOLLOW = computeFollow(symtab,Productions,FIRST,symtab.nonTerminals().front());
    std::cout<<"print FIRST ----------------- \n";
    for(auto & [ntid,ntchar] : FIRST) {
        std::cout<<toString(symtab[ntid].sym()) << " ";
        for(auto & uc : ntchar) {
            std::cout<<toString(uc)<<" ";
        }
        std::cout<<" "<<std::endl;
    }
    std::cout<<"print FOLLOW ----------------- \n";
    for(auto & [ntid,ntchar] : FOLLOW) {
        std::cout<<toString(symtab[ntid].sym()) << " ";
        for(auto & uc : ntchar) {
            std::cout<<toString(uc)<<" ";
        }
        std::cout<<" "<<std::endl;
    }

    
    // // Initiation
    // states.front().push_back({ 0, 0 });
    // generateClosure(states.front());

    // for (size_t i = 0; i < states.size(); ++i) {
    //     for (size_t j = 0; j < states.at(i).size(); ++j) {
    //         if (states.at(i).at(j).dot
    //             >= productions.at(states.at(i).at(j).production).symbols.size())
    //             continue;
    //         generateState(
    //             states.at(i),
    //             productions.at(states.at(i).at(j).production).symbols.at(states.at(i).at(j).dot));
    //     }
    // }
}


