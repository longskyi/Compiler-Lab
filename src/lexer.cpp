#include<iostream>
#include<memory>
#include<memory.h>
#include<string>
#include<algorithm>
#include<vector>
#include<functional>
#include"stringUtil.h"
#include"lexer.h"

namespace Lexer
{

bool LexerInit = false;

using token = std::pair<std::string,std::string>;
const std::vector<token> wordkey = 
    {{"INT","int"},
    {"FLOAT","float"},
    {"VOID","void"},
    {"IF","if"},
    {"ELSE","else"},
    {"WHILE","while"},
    {"RETURN","return"},
    {"PRINT","print"} };
const std::vector<token> skey = 
    {{"LBR","{"},
    {"LBK","["},
    {"RBK","]"},
    {"RBR","}"},
    {"LPA","("},
    {"RPA",")"},
    {"SCO",";"},
    {"CMA",","},
    {"ADD","+"},
    {"MUL","*"},
    {"DIV","/"},
    {"AND","&&"},
    {"OR","||"},
    {"ROP","<"},
    {"ROP","<="},
    {"ROP",">"},
    {"ASG","="},
    {"SKIP"," "},
    {"SKIP","\n"},
    {"SKIP","\r"},
    {"SKIP","\t"} };


using state = int;
#define mSTART 0
#define mID 1
#define mNUM 2
#define mFLO 3

typedef struct node_t
{
    bool acce;
    std::string type;
}node_t;

node_t nodes[100];
int move[120][256];
int avai_node = 4;


void insert_word_key(const std::vector<token> keys)
{
    for(auto & key : keys)
    {
        state curr_state = mSTART;
        for(auto & ch : key.second)
        {
            if(move[curr_state][ch] == -1 || move[curr_state][ch] == mID)
            {
                //分配一个新节点
                move[curr_state][ch] = avai_node;
                curr_state = avai_node;
                nodes[curr_state].acce = true;
                nodes[curr_state].type = "ID";
                avai_node++;
                for(int i = 'a',j='A' ; i<='z' ; )
                {
                    move[curr_state][i] = mID;
                    move[curr_state][j] = mID;
                    i++; j++;
                }
                for(int i = '0';i<='9';i++)
                {
                    move[curr_state][i] = mID;
                }
            }
            else
            {
                curr_state = move[curr_state][ch];
            }
            if(&ch == &(key.second.back()))
            {
                if(! nodes[curr_state].acce || nodes[curr_state].type == "ID")
                {
                    //后插入的不覆盖
                    nodes[curr_state].acce = true;
                    nodes[curr_state].type = key.first;
                }
            }
        }
    }
}

void insert_noWord_key(const std::vector<token> keys)
{
    //插入非word类节点
    for(auto & key : keys)
    {
        state curr_state = mSTART;
        for(auto & ch : key.second)
        {
            if(move[curr_state][ch] == -1)
            {
                //分配一个新节点
                move[curr_state][ch] = avai_node;
                curr_state = avai_node;
                nodes[curr_state].acce = false;
                nodes[curr_state].type = "";
                avai_node++;
            }
            else
            {
                curr_state = move[curr_state][ch];
            }
            if(&ch == &(key.second.back()))
            {
                if(! nodes[curr_state].acce)
                {
                    //后插入的不覆盖
                    nodes[curr_state].acce = true;
                    nodes[curr_state].type = key.first;
                }
            }
        }
    }
}

void insert_id()
{
    nodes[mID].acce = true;
    nodes[mID].type = "ID";
    for(int i = 'a';i<='z';i++) {
        if(move[mSTART][i]== -1) move[mSTART][i] = mID;
    }
    for(int i = 'A';i<='Z';i++) {
        if(move[mSTART][i]== -1) move[mSTART][i] = mID;
    }
    for(int i = 'a',j='A' ; i<='z' ; )
    {
        move[mID][i] = mID;
        move[mID][j] = mID;
        i++; j++;
    }
    for(int i = '0';i<='9';i++)
    {
        move[mID][i] = mID;
    }
    
}

void insert_num()
{
    for(int i = '0';i<='9';i++)
    {
        move[mSTART][i] = mNUM;
    }
    state add = move[mSTART]['+'];
    for(int i = '0';i<='9';i++)
    {
        move[add][i] = mNUM;
    }
    state negN = move[mSTART]['-'] = avai_node;
    nodes[avai_node].acce = false;
    nodes[avai_node].type = "";
    avai_node++;
    for(int i = '0';i<='9';i++)
    {
        move[negN][i] = mNUM;
        move[mNUM][i] = mNUM;
    }
    nodes[mNUM].acce = true;
    nodes[mNUM].type = "NUM";
}

void insert_flo()
{
    state curr = avai_node;
    avai_node++;
    nodes[curr].acce = false;
    nodes[curr].type = "FLO TMP . NO ACCEPT";
    move[mSTART]['.'] = curr;
    move[move[mSTART]['-']]['.'] = curr;
    move[move[mSTART]['+']]['.'] = curr;

    nodes[mFLO].acce = true;
    nodes[mFLO].type = "FLO";
    move[mNUM]['.'] = mFLO;

    for(int i = '0' ; i<= '9';i++) {
        move[mFLO][i] = mFLO;
        move[curr][i] = mFLO;
    }
    
}

typedef struct mScanner_ret
{
    std::string type;
    std::string value;
    size_t next_start;
}mScanner_ret;

mScanner_ret scannerAgent(const std::string & input , size_t start_index)
{
    mScanner_ret ret;
    if(start_index == input.size())
    {
        ret.type = "EOF";
        ret.value = "EOF";
        return ret;
    }
    int lastloc = start_index-1; //上一个合法的字符末尾index（含）
    size_t curr_index = start_index;
    state curr_state = mSTART;
    std::function<void(void)> scanner = [&input,&start_index,&lastloc,&ret,&curr_index,&curr_state,&scanner] {
        char ch = input[curr_index];
        //std::cout<<ch;
        if(move[curr_state][ch] == -1) {
            return;
        }
        //移动
        curr_state = move[curr_state][ch];
        if(nodes[curr_state].acce)
        {
            ret.type = nodes[curr_state].type;
            ret.value = ret.value + input.substr(lastloc+1,curr_index - lastloc); //拼接字符串
            lastloc = curr_index;
        }
        curr_index++;
        scanner();
        return;
    };
    scanner();
    ret.next_start = lastloc+1;
    if(lastloc < start_index)
    {
        ret.type="ERR";
        ret.next_start = start_index+1;
    }
    return ret;

}

mScanner_ret scannerAgentU8(const std::u8string & u8input ,const std::string & input ,  size_t start_index)
{
    auto u8head = [](uint8_t ch) -> int {
        int char_len;
        if ((ch & 0x80) == 0x00) {        // 1-byte
                char_len = 1;
        } else if ((ch & 0xE0) == 0xC0) {  // 2-byte
            char_len = 2;
        } else if ((ch & 0xF0) == 0xE0) {  // 3-byte
            char_len = 3;
        } else if ((ch & 0xF8) == 0xF0) {  // 4-byte
            char_len = 4;
        } else {
            std::cerr<<"Scanner Decode UTF8 ERROR";
        }
        return char_len;
    };
    mScanner_ret ret;
    if(start_index == input.size())
    {
        ret.type = "EOF";
        ret.value = "EOF";
        return ret;
    }
    int lastloc = start_index-1; //上一个合法的字符末尾index（含）
    size_t curr_index = start_index;
    state curr_state = mSTART;
    std::function<void(void)> scanner = [&u8head,&input,&start_index,&lastloc,&ret,&curr_index,&curr_state,&scanner] {
        uint8_t ch = input[curr_index];
        int char_len = u8head(ch);
        if(char_len == 1)
        {
            //std::cout<<ch;
            if(move[curr_state][ch] == -1) {
                return;
            }
            //移动
            curr_state = move[curr_state][ch];
            if(nodes[curr_state].acce)
            {
                ret.type = nodes[curr_state].type;
                ret.value = ret.value + input.substr(lastloc+1,curr_index - lastloc); //拼接字符串
                lastloc = curr_index;
            }
            curr_index++;
            scanner();
            return;
        }
        else {
            if(ret.value.empty()) {
                //新进来U8符合 ID
                ret.type = "ID";
                ret.value = ret.value + input.substr(lastloc+1 ,curr_index + char_len-1 - lastloc);
                curr_state = mID;
                lastloc = curr_index + char_len - 1;
            }
            else {
                char c = ret.value[0];
                if(u8head(uint8_t(c)) > 1) 
                {
                    ret.type = "ID";
                    ret.value = ret.value + input.substr(lastloc+1 ,curr_index + char_len -1  - lastloc);
                    curr_state = mID;
                    lastloc = curr_index + char_len - 1;
                }
                else if( ('a' <= c && c <= 'z' ) || ('A' <= c && c <= 'Z') ) {
                    ret.type = "ID";
                    ret.value = ret.value + input.substr(lastloc+1 ,curr_index + char_len -1 - lastloc);
                    curr_state = mID;
                    lastloc = curr_index + char_len - 1;
                }
                else {
                    //非合法ID
                    return;
                }
            }
            curr_index += char_len; 
            if(curr_index >= input.length()) return;
            scanner();
            return;
        }
        
    };
    scanner();
    ret.next_start = lastloc+1;
    if(lastloc < start_index)
    {
        //没有成功推进
        ret.type="ERR";
        ret.next_start = start_index+1;
    }
    return ret;

}

void init_Lexer() {

    if(LexerInit) {
        return;
    }
    memset(move,-1,sizeof(move));
    for(int i = 0 ; i<60 ; i++)
    {
        nodes[i].acce = false;
        nodes[i].type = "";
    }
    
    insert_word_key(wordkey); //插入会跟ID有接触的
    insert_noWord_key(skey); //插入跟ID没关系的
    insert_id(); //id = [a-zA-Z][a-zA-Z0-9]*
    insert_num(); // num = [+-]?[0-9]+
    insert_flo(); // FLO = [+-]?[0-9]*.[0-9]+|[+-]?[0-9].[0-9]*
    LexerInit = true;

}



/**
 * @attention 确保末尾加上[END,$]
 */
std::vector<scannerToken_t> scan(const std::u8string & u8input) {
    init_Lexer();
    try
    {
        auto len = u8len(u8input); //非合法utf8编码出错会throw
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    std::vector<scannerToken_t> ret;
    size_t st = 0;
    const std::string input = toString(u8input);
    while(1)
    {
        auto lexret= scannerAgentU8(u8input,input,st);
        if(lexret.type != "SKIP" && lexret.type != "EOF" ) {
            ret.emplace_back(toU8str(lexret.type),toU8str(lexret.value));
            //std::cout<<"("<<lexret.type<<",\""<<lexret.value<<"\")";
        }   
        if(lexret.type == "EOF") break;
        st = lexret.next_start;
    }
    ret.push_back({u8"END",u8"$"});
    
    return ret;
}

int test_main_u8()
{
    std::string myprogram = R"(
    int 原始(int 这是x;) {
        这是y=这是x+5;
        return 这是y};
    void 嗨嗨嗨，你在干什么(int y;) {
        int 𰻞;
        void bar(int x; int soo();) {
        if(x>3) bar(x/3,soo(),) else 𰻞 = soo(x);
        print 𰻞};
        bar(y,raw(),)};
    foo(6,)
    )";
    auto ss = Lexer::scan(toU8str(myprogram));
    for(auto q : ss) {
        std::cout<<"["<<toString(q.type)<<","<<toString(q.value)<<"]";
    }
    return 0;
}


int test_main2()
{
    init_Lexer();
    std::string myprogram = R"(
    int raw(int x;) {
        y=x+5;
        return y};
    void foo(int y;) {
        int z;
        void bar(int x; int soo();) {
        if(x>3) bar(x/3,soo(),) else z = soo(x);
        print z};
        bar(y,raw(),)};
    foo(6,)
    )";
    std::string myprogram2 = R"(
    1
    5
    id if 485 841.6541 www
    )";
    std::string myprogram3 = R"(
    while(true){int a=0;}
    )";
    size_t st = 0;
    while(1)
    {
        auto lexret= scannerAgent(myprogram3,st);
        if(lexret.type != "SKIP")   std::cout<<"("<<lexret.type<<",\""<<lexret.value<<"\")";
        if(lexret.type == "EOF") break;
        st = lexret.next_start;
    }
    return 0;   
}

} // namespace Lexer


