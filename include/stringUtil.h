#ifndef LCMP_STRING_UTIL_HEADER
#define LCMP_STRING_UTIL_HEADER
#include <string>
#include <concepts>
#include <vector>
#include <ranges>
#include <string>
#include <stdexcept>
#include <cstdint>

template<typename T>
concept HasCStr = requires(T t) {
    { t.c_str() } -> std::convertible_to<const typename T::value_type*>;
};

template<typename T>
concept HasCData = requires(T t) {
    { t.data() } -> std::convertible_to<const typename T::value_type*>;
    { t.size() } -> std::convertible_to<std::size_t>;
};

/**
 * @brief 转成std::string
 */
template<typename T>
requires HasCStr<T>
std::string toString(T str) {
    return std::string(reinterpret_cast<const char*>(str.c_str()));
}

inline std::string_view toString_view(const std::u8string & u8str) {
    return std::string_view(reinterpret_cast<const char*>(u8str.c_str()));
}

template<typename T>
requires (HasCData<T> && !HasCStr<T>)
std::string toString(T str) {
    using View = std::basic_string_view<typename T::value_type>;
    if constexpr (std::is_convertible_v<View, std::string_view>) {
        return std::string(std::string_view(str.data(), str.size()));
    } else {
        return std::string(str.begin(), str.end());
    }
}

template<typename T>
std::u8string toU8str(T str) {
    return std::u8string(reinterpret_cast<const char8_t *>(str.c_str()));
}

/** 
 * @brief 将 UTF-8 字符串拆分为单个 UTF-8 字符
 * @param str 输入的 UTF-8 字符串
 * @return 包含所有 UTF-8 字符的 vector
 * @throws std::runtime_error 如果遇到无效的 UTF-8 序列
*/
std::vector<std::u8string> extract_utf8_chars(const std::u8string& str);




/**
 * @brief 返回给定u8string的utf-8 char的std::u8string_view
 */
inline auto utf8_chars_view(const std::u8string& str) {
    return std::views::transform(
        std::views::iota(0u, str.size()) | 
        std::views::filter([&](size_t i) {
            uint8_t c = static_cast<uint8_t>(str[i]);
            return (c & 0xC0) != 0x80; // 不是 continuation byte 才保留
        }),
        [&](size_t i) -> std::u8string_view {
            uint8_t c = static_cast<uint8_t>(str[i]);
            size_t char_len = 0;

            if ((c & 0x80) == 0x00) {        // 1-byte
                char_len = 1;
            } else if ((c & 0xE0) == 0xC0) {  // 2-byte
                char_len = 2;
            } else if ((c & 0xF0) == 0xE0) {  // 3-byte
                char_len = 3;
            } else if ((c & 0xF8) == 0xF0) {  // 4-byte
                char_len = 4;
            } else {
                throw std::runtime_error("Invalid UTF-8: bad leading byte");
            }

            if (i + char_len > str.size()) {
                throw std::runtime_error("Invalid UTF-8: truncated character");
            }

            for (size_t j = 1; j < char_len; ++j) {
                if ((static_cast<uint8_t>(str[i + j]) & 0xC0) != 0x80) {
                    throw std::runtime_error("Invalid UTF-8: bad continuation byte");
                }
            }

            return std::u8string_view(&str[i], char_len);
        }
    );
}

/**
 * @brief string组件简单测试程序
 */
int string_test_main();

size_t u8len(const std::u8string & str);

std::u8string head_u8char(const std::u8string & str);

std::u8string replace(const std::u8string& src, const std::u8string& pattern, const std::u8string& alt);

#endif