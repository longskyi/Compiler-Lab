#ifndef LCMP_GRAMMAR_READ_HEADER
#define LCMP_GRAMMAR_READ_HEADER
#include<json.hpp>
#include<filesystem>
#include"SyntaxType.h"
#include"templateUtil.h"


template <typename Tag, typename T>
void to_json(nlohmann::json& j, const StrongId<Tag, T>& id) {
    j = id.value; // 直接使用 T 的类型
}

template <typename Tag, typename T>
void from_json(const nlohmann::json& j, StrongId<Tag, T>& id) {
    id.value = j.get<T>(); // 按 T 的类型解析
}

namespace LCMPFileIO
{
    using json = nlohmann::json;
    
    //自定义了json转换的类型
    using json_supported_types = std::tuple<int, float ,char>;

    template <typename T>
    concept SupportedType = IsInTuple<T,json_supported_types>;

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

        static void serialize(json& j, const SymbolTable& st) {
            j = {{"next_index", st.next_index_}, {"scope", st.nonTerminalId}};
        }

        // // 自定义类型反序列化
        // static void deserialize(const json& j, SymbolTable& st) {
        //     j.at("symbols").get_to(st.symbols);
        //     j.at("scope").get_to(st.scope);
        // }
    };


    U8StrProduction parseProduction(const std::u8string& line);
    ForceReducedProd parseProductionR(const std::u8string& line);
    std::vector<ForceReducedProd> parseProdFileR (const std::filesystem::path & inputPath);
    int test_main_strProd(const std::filesystem::path& inputPath);
} // namespace LCMPFileIO


void readTerminals(SymbolTable & symtable,std::filesystem::path terminalPath);

void readProductionRule(SymbolTable & symtable,std::vector<Production> & productions , std::filesystem::path rulePath);


inline void to_json(nlohmann::json & j, const LCMPFileIO::SupportedType auto & obj) {
    LCMPFileIO::JsonConverter::serialize(j,obj);
}
inline void from_json(nlohmann::json & j, LCMPFileIO::SupportedType auto & obj) {
    LCMPFileIO::JsonConverter::deserialize(j,obj);
}

#endif //LCMP_GRAMMAR_READ_HEADER