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

void to_json(nlohmann::json & j, const action & obj);

void from_json(const nlohmann::json& j, action& obj);

void to_json(nlohmann::json& j, const ASTbaseContent & obj);

inline void from_json(const nlohmann::json& j, SymbolTable & st) {
    LCMPFileIO::JsonConverter::deserializeP(j,st);
}

void from_json(const nlohmann::json& j, Production& obj);

void from_json(const nlohmann::json& j, ASTbaseContent& obj);



namespace LCMPFileIO
{

    U8StrProduction parseProduction(const std::u8string_view& line);
    ForceReducedProd parseProductionR(const std::u8string& line);
    std::vector<ForceReducedProd> parseProdFileR (const std::filesystem::path & inputPath);
    int test_main_strProd(const std::filesystem::path& inputPath);
    std::u8string formatProduction(const Production& prod , const SymbolTable& symtab);
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