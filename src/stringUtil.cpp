#include"stringUtil.h"
#include<iostream>

std::vector<std::u8string> extract_utf8_chars(const std::u8string& str) {
    auto chars_view = utf8_chars_view(str);
    std::vector<std::u8string> res;
    for(auto u8view : chars_view) {
        std::u8string u8char(u8view);
        res.push_back(u8char);
    }
    return res;
}

int string_test_main() {
    std::u8string text = u8"你好，世界！🌍";
    for (auto ch : utf8_chars_view(text)) {
        std::cout << toString(ch) << " ";
    }
    // 输出：你 好 ， 世 界 ！ 🌍
    return 0;
}

size_t u8len(const std::u8string & str) {
    auto views = utf8_chars_view(str);
    size_t ret = 0;
    for(const auto & q : views) {
        ret++;
    }
    return ret;
}

std::u8string head_u8char(const std::u8string & str) {
    auto views = utf8_chars_view(str);
    auto sview = *views.begin();
    return std::u8string(sview);
}