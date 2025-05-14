#ifndef LCMP_DFA_HEADER
#define LCMP_DFA_HEADER
#include "templateUtil.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include <optional>
#include <memory>




template <typename StateIDType, typename Symbol>
requires    Hashable<StateIDType> &&  Hashable<Symbol> &&
            EqualityComparable<StateIDType> && EqualityComparable<Symbol>
class DFA {
private:
    StateIDType current_StateIDType;
    StateIDType initial_StateIDType;
    std::unordered_set<StateIDType> accepting_StateIDTypes;

    // 转移函数：StateIDType × Symbol -> StateIDType
    std::unordered_map<StateIDType, std::unordered_map<Symbol, StateIDType>> transition_function;

public:
    // 构造函数
    DFA(StateIDType initial_StateIDType_, const std::unordered_set<StateIDType>& accepting_StateIDTypes_)
        : initial_StateIDType(initial_StateIDType_), current_StateIDType(initial_StateIDType_), accepting_StateIDTypes(accepting_StateIDTypes_) {}

    void modify(StateIDType initial_StateIDType_, const std::unordered_set<StateIDType>& accepting_StateIDTypes_) {
        initial_StateIDType = initial_StateIDType_;
        current_StateIDType = initial_StateIDType_;
        accepting_StateIDTypes = accepting_StateIDTypes_;
    }

    // 添加转移规则-可覆盖
    void add_transition(StateIDType from, Symbol symbol, StateIDType to) {
        transition_function[from][symbol] = to;
    }

    // 处理输入符号
    std::optional<StateIDType> process_symbol(Symbol symbol) {
        auto& transitions = transition_function[current_StateIDType];
        auto it = transitions.find(symbol);
        if (it == transitions.end()) {
            return std::nullopt;
        }
        current_StateIDType = it->second;
        return current_StateIDType;
    }

    std::optional<StateIDType> query_rule(StateIDType currStateIDType , Symbol symbol) {
        if(transition_function[currStateIDType].count(symbol)) {
            return transition_function[currStateIDType][symbol];
        }
        return std::nullopt;
    }

    // 判断当前状态是否接受
    bool is_accepting() const {
        return accepting_StateIDTypes.count(current_StateIDType) > 0;
    }

    // 重置 DFA 到初始状态
    void reset() {
        current_StateIDType = initial_StateIDType;
    }

    // 处理整个输入序列
    bool process_input(const std::vector<Symbol>& input) {
        reset();
        for (Symbol symbol : input) {
            process_symbol(symbol);
        }
        return is_accepting();
    }
};

inline void DFA_test_main() {

    DFA<int, int> dfa(0, {2});  // 初始状态 0，接受状态 2

    dfa.add_transition(0, 1, 1);  // 0 --1--> 1
    dfa.add_transition(1, 2, 2);  // 1 --2--> 2

    std::vector<int> input = {1, 2};
    std::cout << "Input accepted: " << dfa.process_input(input) << std::endl; // true

}

#endif