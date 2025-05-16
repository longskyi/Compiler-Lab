#ifndef LCMP_LEXER_HEADER
#define LCMP_LEXER_HEADER

#include<string>
#include<vector>

namespace Lexer
{
    typedef struct scannerToken_t
    {
        std::u8string type;
        std::u8string value;
    }scannerToken_t;

    std::vector<scannerToken_t> scan(const std::u8string & u8input);
    int test_main_u8();
    
} // namespace Lexer




#endif