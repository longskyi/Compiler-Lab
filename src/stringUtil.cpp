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


// 计算部分匹配表（LPS数组）
std::vector<int> computeLPS(const std::u8string& pattern) {
    int n = pattern.length();
    std::vector<int> lps(n, 0);
    int len = 0;
    int i = 1;
    
    while (i < n) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = lps[len - 1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }
    
    return lps;
}

// KMP搜索算法
int kmpSearch(const std::u8string& text, const std::u8string& pattern, size_t start_pos = 0) {
    int m = text.length();
    int n = pattern.length();
    
    if (n == 0) return 0; // 空模式串匹配开始位置
    
    std::vector<int> lps = computeLPS(pattern);
    int i = start_pos; // 文本索引
    int j = 0; // 模式串索引
    
    while (i < m) {
        if (pattern[j] == text[i]) {
            i++;
            j++;
            
            if (j == n) {
                return i - j; // 找到匹配
            }
        } else {
            if (j != 0) {
                j = lps[j - 1];
            } else {
                i++;
            }
        }
    }
    
    return -1; // 未找到匹配
}


std::u8string replace(const std::u8string& src, const std::u8string& pattern, const std::u8string& alt) {
    std::u8string result;
    size_t pos = 0;
    int found;
    
    while ((found = kmpSearch(src, pattern, pos)) != -1) {
        result.append(src, pos, found - pos);
        result.append(alt);
        pos = found + pattern.length();
    }
    result.append(src, pos, src.length() - pos);
    return result;
}