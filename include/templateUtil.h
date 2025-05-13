#ifndef LCMP_TEMPLATEUTIL_HEADER
#define LCMP_TEMPLATEUTIL_HEADER
#include<algorithm>
#include<iostream>
// std::variant语法糖 C++17
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>; 
// 使用办法
// std::variant<int,double,string> q;
// auto visitor = overload {
//     [](int v) ->int { cout<<v; return 1; }
//     [](double v) ->int { cout<<v; return 2;}
//     [](string v) ->int { cout<<v; return 2;}
// };
// int ret = std::visit(visitor,q);


template<typename Tag, typename T = size_t>
struct StrongId {
    T value;
    
    explicit constexpr StrongId(T v = {}) : value(v) {}
    constexpr operator T() const { return value; }
    //C++20
    auto operator<=>(const StrongId&) const = default;
    
    friend std::ostream& operator<<(std::ostream& os, StrongId id) {
        return os << id.value;
    }
};

// 特化哈希
namespace std {
    template<typename Tag, typename T>
    struct hash<StrongId<Tag, T>> {
        size_t operator()(const StrongId<Tag, T>& id) const {
            return hash<T>()(id.operator T());
        }
    };
}


#endif
