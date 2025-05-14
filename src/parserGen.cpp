#include"SyntaxType.h"
#include"stringUtil.h"
#include"templateUtil.h"
#include"fileIO.h"
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
                if(prodo.lhs() != prod.rhs()[state[i].dot_pos] ) continue;
                if(!inState(dpi)) {
                    state.push_back(dpi);
                }
            }
        }
    }
}

/**
 * @brief 新状态生成
 * @param symtab,Productions 符号表 产生式表
 * @param curr_state 当前状态
 * @param symbolId 符号id（包含终结符与非终结符）
 */
std::vector<dotProdc> generateState(const SymbolTable & symtab ,const std::unordered_map<ProductionId,Production> & Productions,const std::vector<dotProdc> &curr_state, const SymbolIdType auto symbolId) {
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

/**
 * @brief 检查新状态是否已存在于状态集合中（使用简单易用的朴素查重算法，O(n3)复杂度）
 * @param states 现有的所有状态集合
 * @param newState 待检查的新状态
 * @return 若存在则返回对应状态的ID，否则返回-1
 */
std::optional<StateId> findExistingState(
    const std::vector<std::vector<dotProdc>>& states,
    const std::vector<dotProdc>& newState) 
{
    if (newState.empty()) return std::nullopt;

    // 第一层遍历：所有现有状态
    for (int existingId = 0; existingId < states.size(); ++existingId) {
        const auto& existingState = states[existingId];
        // 快速检查：状态大小不同则直接跳过
        if (existingState.size() != newState.size()) continue;
        bool isSame = true;
        // 第二层遍历：比较每个dotProdc项
        for (const auto& newItem : newState) {
            bool foundItem = false;
            // 第三层遍历：在现有状态中查找匹配项
            for (const auto& existingItem : existingState) {
                if (existingItem.producId == newItem.producId && 
                    existingItem.dot_pos == newItem.dot_pos) {
                    foundItem = true;
                    break;
                }
            }
            if (!foundItem) {
                isSame = false;
                break;
            }
        }
        
        if (isSame) {
            return StateId(existingId); // 找到完全相同的状态
        }
    }
    
    return std::nullopt; // 未找到重复状态
}

// 修改后的状态生成逻辑，同时处理GOTO
void processNewStateGOTO(
    std::vector<std::vector<dotProdc>>& states,
    std::vector<std::unordered_map<SymbolId, StateId>>& gotoTable,
    const SymbolTable& symtab,
    const std::unordered_map<ProductionId, Production>& Productions,
    const std::vector<dotProdc>& newState,
    const StateId curr_stateId,
    const SymbolId curr_symId) 
{
    // 先查重
    auto existingId = findExistingState(states, newState);

    if (!existingId) {
        // 新状态处理
        StateId newId = StateId(states.size());
        states.push_back(newState);
        // 初始化对应的action和goto表项
        gotoTable.emplace_back();
        gotoTable.at(curr_stateId)[curr_symId] = newId;
    } else {
        gotoTable.at(curr_stateId)[curr_symId] = existingId.value();
    }
}

/* 冲突类型定义 */
enum ConflictType {
    NO_CONFLICT,
    SHIFT_REDUCE,
    REDUCE_REDUCE
};

/**
 * @brief SLR(1)冲突消解策略
 * @return bool 是否成功消解冲突
 * @note SLR(1)只能处理移进-规约冲突(优先移进)
 */
bool resolveSLRConflict(
    const std::vector<std::vector<dotProdc>>& states,
    std::vector<std::unordered_map<SymbolId, action>>& actionTable,
    const SymbolTable& symtab,
    const std::unordered_map<ProductionId, Production>& Productions,
    StateId state,
    SymbolId symbolid,
    action exist,
    action new_action,
    ConflictType conflict_type) {
    // SLR只能处理移进-规约冲突(优先移进)

    auto visitor = overload {
        [](ProductionId v) ->bool  { return false;},
        [](StateId v) ->bool { return true;}
    };

    bool existing_is_shift = std::visit(visitor,exist);
    bool new_is_shift = std::visit(visitor,new_action);

    if (conflict_type == SHIFT_REDUCE) {
        std::cerr << "SLR冲突消解: 选择移进优先" << std::endl;
        // 无论哪种情况，都保留移进动作
        if(existing_is_shift) {
            // 现有是移进，保持不变
            return true;
        }
        else if(new_is_shift) {
            // 新动作是移进，覆盖现有规约
            actionTable[state][symbolid] = new_action;
            return true;
        }
    }
    else {
        std::cerr << "归约-归约冲突，未定义消解方案" << std::endl;
    }
    return false;
}

std::u8string formatProduction(const Production& prod, size_t dot_pos, const SymbolTable& symtab) {
    std::u8string result;
    result += u8"[";
    result += symtab[prod.lhs()].sym();
    result += u8" -> ";
    
    const auto& rhs = prod.rhs();
    for (size_t i = 0; i < rhs.size(); ++i) {
        if (i == dot_pos) {
            result += u8"·";
        }
        result += symtab[rhs[i]].sym();
        if (i != rhs.size() - 1) {
            result += u8" ";
        }
    }
    
    // 处理点在最后的情况
    if (dot_pos == rhs.size()) {
        result += u8"·";
    }
    
    result += u8"]";
    return result;
}

/**
 * @brief 报告冲突详细信息
 */
void reportConflict(
    const std::vector<std::vector<dotProdc>>& states,
    std::vector<std::unordered_map<SymbolId, action>>& actionTable,
    const SymbolTable& symtab,
    const std::unordered_map<ProductionId, Production>& Productions,
    StateId state,
    SymbolId symbolid,
    action exist,
    action new_action,
    ConflictType conflict_type) {

    std::cerr << "冲突: 状态 " << state << " 在符号 '" << toString(symtab[symbolid].sym()) << "' 上 ";
    
    auto visitor = overload {
        [](ProductionId v) ->bool  { return false;},
        [](StateId v) ->bool { return true;}
    };

    bool existing_is_shift = std::visit(visitor,exist);
    bool new_is_shift = std::visit(visitor,new_action);

    switch(conflict_type) {
        case SHIFT_REDUCE:
            std::cerr << "移进-规约冲突\n";
            if (existing_is_shift) {
                // 现有是移进，新的是规约
                std::cerr << "  现有: 移进到状态 " << std::get<StateId>(exist) << "\n";
                //printState(states[(existing_action - productions.size() - 1)],(existing_action - productions.size() - 1));
                std::cerr << "  新动作: 规约产生式 " << std::get<ProductionId>(new_action) << " (";
                std::cerr << toString(formatProduction(Productions.at(std::get<ProductionId>(new_action)),Productions.at(std::get<ProductionId>(new_action)).rhs().size(),symtab));
                std::cerr << ")" << std::endl;
            } else {
                // 现有是规约，新的是移进
                std::cerr << "  现有: 规约产生式 " << std::get<ProductionId>(exist) << " (";
                std::cerr << toString(formatProduction(Productions.at(std::get<ProductionId>(exist)),Productions.at(std::get<ProductionId>(exist)).rhs().size(),symtab));
                std::cerr << ")\n";
                std::cerr << "  新动作: 移进到状态 " << std::get<StateId>(new_action) << "\n";
                //printState(states[(new_action - productions.size() - 1)],(new_action - productions.size() - 1));
            }
            break;
            
        case REDUCE_REDUCE:
            std::cerr << "规约-规约冲突\n";
            std::cerr << "  现有: 规约产生式 " << std::get<ProductionId>(exist) << " (";
            std::cerr << toString(formatProduction(Productions.at(std::get<ProductionId>(exist)),Productions.at(std::get<ProductionId>(exist)).rhs().size(),symtab));
            std::cerr << ")\n";
            std::cerr << "  新动作: 规约产生式 " << std::get<ProductionId>(new_action) << " (";
                std::cerr << toString(formatProduction(Productions.at(std::get<ProductionId>(new_action)),Productions.at(std::get<ProductionId>(new_action)).rhs().size(),symtab));
            std::cerr << ")" << std::endl;
            break;
        default:
            break;
    }
}


/**
 * @brief 检查ACTION表冲突
 * @return ConflictType 冲突类型
 */
ConflictType checkActionConflict(action exist , action new_action) {

    auto visitor = overload {
        [](ProductionId v) ->bool  { return false;},
        [](StateId v) ->bool { return true;}
    };

    bool existing_is_shift = std::visit(visitor,exist);
    bool new_is_shift = std::visit(visitor,new_action);

    // 移进-规约冲突
    if ((existing_is_shift && !new_is_shift) || (!existing_is_shift && new_is_shift)) {
        return SHIFT_REDUCE;
    }
    // 规约-规约冲突
    if (!existing_is_shift && !new_is_shift && exist != new_action) {
        return REDUCE_REDUCE;
    }
    return NO_CONFLICT;
}

/**
 * @brief 生成SLR(1)分析表
 * @return bool 是否成功生成(无不可消解冲突)
 */
int generateSLRTable(
    const std::vector<std::vector<dotProdc>>& states,
    std::vector<std::unordered_map<SymbolId, action>>& actionTable,
    const std::vector<std::unordered_map<SymbolId, StateId>>& gotoTable,
    const SymbolTable& symtab,
    const std::unordered_map<ProductionId, Production>& Productions,
    const std::unordered_map<NonTerminalId, std::unordered_set<std::u8string>> & FOLLOW,
    const NonTerminalId startId)
{
    assert(states.size() == gotoTable.size() );
    actionTable.resize(states.size());
    int conflicts_count = 0;

    for (StateId state_i(0); state_i < states.size(); state_i=StateId(state_i+1))
    {
        for (auto & dotP : states[state_i])
        {
            const Production & prod = Productions.at(dotP.producId); 
            // 可规约项目
            if (dotP.dot_pos == prod.rhs().size())
            {
                // 接受动作
                if (prod.lhs() == startId)
                {
                    if (actionTable[state_i].count(SymbolId(startId))) {
                        std::cerr << "接受状态错误";
                        conflicts_count += 1;
                    }
                    action accept = prod.index();
                    actionTable[state_i][SymbolId(startId)] = accept;
                    continue;
                }
                // 常规规约
                for (const auto & follow_sym : FOLLOW.at(prod.lhs())) {
                    auto symid_ = symtab.find_index(follow_sym);
                    if(!symid_ || !symtab[symid_.value()].is_terminal()) {
                        std::cerr<<"FOLLOW集出错";
                    }
                    auto termid = TerminalId(symid_.value());
                    if (actionTable[state_i].count(SymbolId(termid))) {
                        // 检查冲突
                        action new_action = prod.index();
                        ConflictType conflict = checkActionConflict(
                            actionTable[state_i][SymbolId(termid)], new_action);
                        if (conflict != NO_CONFLICT) {
                            reportConflict(states,actionTable,symtab,Productions,state_i,SymbolId(termid),actionTable[state_i][SymbolId(termid)], new_action,conflict);
                            conflicts_count += 1;
                            // 尝试消解冲突
                            if (!resolveSLRConflict(states,actionTable,symtab,Productions,state_i,SymbolId(termid),actionTable[state_i][SymbolId(termid)], new_action,conflict) ){
                                continue;  // 无法消解，跳过设置
                            } else {
                                conflicts_count -= 1;
                            }
                        }
                        else  {
                            // 无冲突，正常进行归约
                            actionTable[state_i][SymbolId(termid)] = new_action;
                        }
                    } else {
                        // 正常进行规约
                        actionTable[state_i][SymbolId(termid)] = prod.index();
                    }
                }
                continue;
            }

            // 处理移进/转移项目
            TerminalId termid = TerminalId(prod.rhs()[dotP.dot_pos]);
            std::u8string sym = symtab[termid].sym();
            if (symtab[termid].is_terminal()) {  // 终结符 - 移进
                action shift_action = gotoTable[state_i].at(SymbolId(termid));
                
                // 检查冲突
                if (actionTable[state_i].count(SymbolId(termid))) {
                    // 检查冲突
                    action new_action = shift_action;
                    ConflictType conflict = checkActionConflict(
                        actionTable[state_i][SymbolId(termid)], new_action);
                    if (conflict != NO_CONFLICT) {
                        reportConflict(states,actionTable,symtab,Productions,state_i,SymbolId(termid),actionTable[state_i][SymbolId(termid)], new_action,conflict);
                        conflicts_count += 1;
                        // 尝试消解冲突
                        if (!resolveSLRConflict(states,actionTable,symtab,Productions,state_i,SymbolId(termid),actionTable[state_i][SymbolId(termid)], new_action,conflict) ){
                            continue;  // 无法消解，跳过设置
                        } else {
                            conflicts_count -= 1;
                        }
                    }
                    else  {
                        // 无冲突，正常进行移进
                        actionTable[state_i][SymbolId(termid)] = shift_action;
                    }
                } else {
                    // 正常进行移进
                    actionTable[state_i][SymbolId(termid)] = shift_action;
                }
            }
        }
    }
    
    return conflicts_count;
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
    std::vector<std::unordered_map<SymbolId, action>> actionTable;
    std::vector<std::unordered_map<SymbolId,StateId>> gotoTable;
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
    symtab.add_symbol(u8"START",u8"START",u8"",false);
    symtab.add_symbol(u8"$",u8"END",u8"",true);
    //增广文法
    auto pres = Production::create(symtab,u8"START",std::vector<std::u8string>{symtab[symtab.nonTerminals()[0]].sym()});
    Productions.insert({pres.index(),pres});

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

    
    states.emplace_back(std::vector<dotProdc>{{0,pres.index()}});
    generateClosure(symtab,Productions,states[0]);
    gotoTable.emplace_back();
    for(size_t i = 0 ; i < states.size() ; i++) {
        std::unordered_set<SymbolId> symclosure;
        for(auto & dprod : states[i] ) {
            const auto & prod = Productions.at(dprod.producId);
            if(dprod.dot_pos < prod.rhs().size()) {
                symclosure.insert(prod.rhs().at(dprod.dot_pos));
            }
        }
        for(auto & symid : symclosure) {
            auto nSt = generateState(symtab,Productions,states[i],symid);
            processNewStateGOTO(states, gotoTable, symtab, Productions , nSt , StateId(i),symid);
        }
    }
    
    std::cout<<"print STATE ---------------\n";
    for(size_t i = 0; i < states.size(); i++) {
        std::cout<<i<<": ";
        for(auto p : states[i]) {
            auto prod_it = Productions.find(p.producId);
            if(prod_it == Productions.end()) {
                std::cerr << "Invalid production ID in state";
                continue;
            }
            const auto& prod = prod_it->second;
            std::cout << toString(formatProduction(prod, p.dot_pos, symtab)) << " ";
        }
        std::cout<<std::endl;
    }

    bool success = generateSLRTable(states,actionTable,gotoTable,symtab,Productions,FOLLOW,NonTerminalId(symtab.find_index(u8"START").value()));
    
}


