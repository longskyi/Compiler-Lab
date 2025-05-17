#ifndef LCMP_GRAMMAR_READ_HEADER
#define LCMP_GRAMMAR_READ_HEADER
#include<json.hpp>
#include<filesystem>
#include"SyntaxType.h"
#include"parserGen.h"
#include"templateUtil.h"


template <typename Tag, typename T>
void to_json(nlohmann::json& j, const StrongId<Tag, T>& id) {
    j = id.value; // 直接使用 T 的类型
}

template <typename Tag, typename T>
void from_json(const nlohmann::json& j, StrongId<Tag, T>& id) {
    id.value = j.get<T>(); // 按 T 的类型解析
}


namespace LCMPFileIO {
    using json = nlohmann::json;
    
    //自定义了json转换的类型
    using json_supported_types = std::tuple<int>;

    template <typename T>
    concept SupportedType = IsInTuple<T,json_supported_types>;
}




namespace LCMPFileIO
{
    
    struct mSymbolWrapper {
        std::u8string sym_;
        std::u8string token_type_;
        std::u8string pattern_;
        bool isTerminal_;
        size_t index_;
        // 提供默认构造函数
        mSymbolWrapper() = default;
        // 转换函数
    };

    class JsonConverter {
    public:
        static void serialize(json& j, const int & obj) {
            // 默认实现（可选）
            j = obj; // 如果类型本身支持直接转换
        }
        static void deserialize(json& j, const int & obj) {
            // 默认实现（可选）
            j = obj; // 如果类型本身支持直接转换
        }
        inline static auto serializeP(const Symbol& st) {
            return std::make_tuple(st.index_ , st.isTerminal_ , st.pattern_ , st.sym_ , st.tokentype_);
        }
        inline static auto deserializeP(const nlohmann::json& j,Symbol& obj) {
            obj.index_ = j.at("index").get<decltype(obj.index_)>();
            obj.isTerminal_ = j.at("isTerminal").get<decltype(obj.isTerminal_)>();
            obj.pattern_ = j.at("pattern").get<decltype(obj.pattern_)>();
            obj.sym_ = j.at("sym").get<decltype(obj.sym_)>();
            obj.tokentype_ = j.at("tokentype").get<decltype(obj.tokentype_)>();
        }
        static void deserializeP(const nlohmann::json& j, SymbolTable& st);
        static void serialize(json& j, const SymbolTable & st);
        static void serialize(json& j, const Production & obj);
        static void serialize(json& j, const dotProdc & obj);
        inline static Symbol Symbol_Construct_helper(std::u8string sym_,std::u8string token_type_, std::u8string pattern_,
            bool isTerminal_,size_t index_) {
            return Symbol(sym_,token_type_,pattern_,isTerminal_,index_);
        }
        inline static Production Production_Construct_helper(NonTerminalId lhs_index_ , std::vector<SymbolId> rhs_indices_ , ProductionId index_) {
            Production::next_index_++;
            return Production(lhs_index_,rhs_indices_,index_);
        }
    };
    
}

template<typename T1>
void to_json(nlohmann::json & j , const StrongId<T1> & obj) {
    j = obj.value;
}

template<typename T1>
void from_json(nlohmann::json & j , const StrongId<T1> & obj) {
    obj.value = j;
}

inline void to_json(nlohmann::json & j, const Symbol & obj) {
    auto [i1,i2,i3,i4,i5] = LCMPFileIO::JsonConverter::serializeP(obj);
    j = {
        {"index",i1},
        {"isTerminal",i2},
        {"pattern",i3},
        {"sym",i4},
        {"tokentype",i5},
    };
}


inline void from_json(const nlohmann::json & j, Symbol & obj) {
    LCMPFileIO::JsonConverter::deserializeP(j,obj);
}

inline void LCMPFileIO::JsonConverter::deserializeP(const nlohmann::json& j, SymbolTable& st) {
    j.at("next_index").get_to(st.next_index_);
    j.at("nonTerminalId").get_to(st.nonTerminalId);
    j.at("name_to_index").get_to(st.name_to_index_);
    std::vector<nlohmann::json> symbols;    //提供默认构造函数
    j.at("symbols").get_to(symbols);
    for(auto & p : symbols) {
        Symbol a(u8"",u8"",u8"",false,0);
        from_json(p,a);
        st.symbols_.emplace_back(std::move(a));
    }
}


inline void LCMPFileIO::JsonConverter::serialize(json& j, const SymbolTable& st) {
    j = {
    {"next_index", st.next_index_},
    {"nonTerminalId", st.nonTerminalId},
    {"name_to_index",st.name_to_index_},
    {"symbols",st.symbols_}
    };
}

inline void LCMPFileIO::JsonConverter::serialize(json& j, const Production & obj) {
    j = {
        {"index",obj.index_},
        {"lhs_index",obj.lhs_index_},
        {"next_index",obj.next_index_},
        {"rhs_indices",obj.rhs_indices_},
    };
}

inline void LCMPFileIO::JsonConverter::serialize(json& j, const dotProdc & obj) {
    j = {
        {"dot_pos",obj.dot_pos},
        {"prodcId",obj.producId},
    };
}

inline void to_json(nlohmann::json & j, const Production & obj) {
    LCMPFileIO::JsonConverter::serialize(j,obj);
}

inline void to_json(nlohmann::json & j, const dotProdc & obj) {
    LCMPFileIO::JsonConverter::serialize(j,obj);
}

inline void from_json(const nlohmann::json& j, dotProdc& obj) {
    j.at("dot_pos").get_to(obj.dot_pos);
    j.at("prodcId").get_to(obj.producId);
}

inline void to_json(nlohmann::json & j, const SymbolTable & obj) {
    LCMPFileIO::JsonConverter::serialize(j,obj);
}

inline void to_json(nlohmann::json & j, const action & obj) {
    auto visitor = overload {
        [&j](ProductionId v) {
            j = {{"type",1},{"val",v}};
        },
        [&j](StateId v) {
            j = {{"type",2},{"val",v}};
        }
    };
    std::visit(visitor,obj);
}

inline void from_json(const nlohmann::json& j, action& obj) {
    int type = j.at("type").get<int>();
    if (type == 1) {
        obj = j.at("val").get<ProductionId>();
    } else if (type == 2) {
        obj = j.at("val").get<StateId>();
    } else {
        throw std::runtime_error("Invalid action type");
    }
}





inline void to_json(nlohmann::json& j, const ASTbaseContent & obj) {
    j = {
    {"gotoTable",obj.gotoTable},
    {"Productions",obj.Productions},
    {"actionTable",obj.actionTable},
    {"states",obj.states},
    {"symtab",obj.symtab}};
}

inline void from_json(const nlohmann::json& j, SymbolTable & st) {
    LCMPFileIO::JsonConverter::deserializeP(j,st);
}

inline void from_json(const nlohmann::json& j, Production& obj) {
    NonTerminalId lhs_index_;
    std::vector<SymbolId> rhs_indices_;
    ProductionId index_;
    size_t next_index_;
    j.at("index").get_to(index_);
    j.at("lhs_index").get_to(lhs_index_);
    //j.at("next_index").get_to(next_index_);
    j.at("rhs_indices").get_to(rhs_indices_);
    obj = LCMPFileIO::JsonConverter::Production_Construct_helper(lhs_index_,rhs_indices_,index_);
}

inline void from_json(const nlohmann::json& j, ASTbaseContent& obj) {
    j.at("gotoTable").get_to(obj.gotoTable);
    j.at("actionTable").get_to(obj.actionTable);
    j.at("states").get_to(obj.states);
    j.at("symtab").get_to(obj.symtab);
    std::unordered_map<ProductionId, nlohmann::json> prods;
    j.at("Productions").get_to(prods);
    obj.Productions.clear();
    NonTerminalId lhs_index_;
    std::vector<SymbolId> rhs_indices_;
    ProductionId index_;
    auto prod = LCMPFileIO::JsonConverter::Production_Construct_helper(lhs_index_,rhs_indices_,index_);
    for(auto p : prods) {
        from_json(p.second,prod);
        obj.Productions.insert(std::make_pair(p.first,prod));
    }
}



namespace LCMPFileIO
{
       
    // // 自定义类型反序列化
    // static void deserialize(const json& j, SymbolTable& st) {
    //     j.at("symbols").get_to(st.symbols);
    //     j.at("scope").get_to(st.scope);
    // }

    U8StrProduction parseProduction(const std::u8string& line);
    ForceReducedProd parseProductionR(const std::u8string& line);
    std::vector<ForceReducedProd> parseProdFileR (const std::filesystem::path & inputPath);
    int test_main_strProd(const std::filesystem::path& inputPath);
} // namespace LCMPFileIO


inline void to_json(nlohmann::json & j, const LCMPFileIO::SupportedType auto & obj) {
    LCMPFileIO::JsonConverter::serialize(j,obj);
}
inline void from_json(nlohmann::json & j, LCMPFileIO::SupportedType auto & obj) {
    LCMPFileIO::JsonConverter::deserialize(j,obj);
}


void readTerminals(SymbolTable & symtable,std::filesystem::path terminalPath);

void readProductionRule(SymbolTable & symtable,std::vector<Production> & productions , std::filesystem::path rulePath);




#endif //LCMP_GRAMMAR_READ_HEADER