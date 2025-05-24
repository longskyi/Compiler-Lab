#ifndef LCMP_ASM_GEN_HEADER
#define LCMP_ASM_GEN_HEADER
#include "irGen.h"


namespace ASM {

using std::string;
using std::u8string;

enum X64Register {
    
    RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
    R8, R9, R10, R11, R12, R13, R14, R15,
    
    EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP,
    R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D,
    
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
    
    RIP, FLAGS,
    
    NONE,
};

inline string Eformat(X64Register reg) {
    switch (reg) {
        // 64-bit registers
        case RAX: return "%rax";
        case RBX: return "%rbx";
        case RCX: return "%rcx";
        case RDX: return "%rdx";
        case RSI: return "%rsi";
        case RDI: return "%rdi";
        case RBP: return "%rbp";
        case RSP: return "%rsp";
        case R8:  return "%r8";
        case R9:  return "%r9";
        case R10: return "%r10";
        case R11: return "%r11";
        case R12: return "%r12";
        case R13: return "%r13";
        case R14: return "%r14";
        case R15: return "%r15";

        // 32-bit registers
        case EAX:  return "%eax";
        case EBX:  return "%ebx";
        case ECX:  return "%ecx";
        case EDX:  return "%edx";
        case ESI:  return "%esi";
        case EDI:  return "%edi";
        case EBP:  return "%ebp";
        case ESP:  return "%esp";
        case R8D:  return "%r8d";
        case R9D:  return "%r9d";
        case R10D: return "%r10d";
        case R11D: return "%r11d";
        case R12D: return "%r12d";
        case R13D: return "%r13d";
        case R14D: return "%r14d";
        case R15D: return "%r15d";

        // XMM registers
        case XMM0:  return "%xmm0";
        case XMM1:  return "%xmm1";
        case XMM2:  return "%xmm2";
        case XMM3:  return "%xmm3";
        case XMM4:  return "%xmm4";
        case XMM5:  return "%xmm5";
        case XMM6:  return "%xmm6";
        case XMM7:  return "%xmm7";
        case XMM8:  return "%xmm8";
        case XMM9:  return "%xmm9";
        case XMM10: return "%xmm10";
        case XMM11: return "%xmm11";
        case XMM12: return "%xmm12";
        case XMM13: return "%xmm13";
        case XMM14: return "%xmm14";
        case XMM15: return "%xmm15";

        // Special registers
        case RIP:   return "%rip";
        case FLAGS: return "%flags";

        // Default case
        case NONE:
        default:
            return "";
    }
}

struct OperandContext{
    bool isStackPtr = false;    //可通过rbp+来计算的情况
    int rbpOffset;  //operand值可通过rsp/rbp计算
    int StackOffset;    //operand存储在栈上位置（rbp-StackOffset)
    size_t startIdx;    //从某条ir指令开始活跃
    bool will_escape = false;    //生命周期逃离当前基本块
    std::vector<size_t> usedIdx;    //在某条ir指令被使用
    std::unordered_set<X64Register> requireReg; //可使用寄存器
    std::unordered_set<X64Register> mustUseReg; //必须使用的寄存器
};

class X64RegStatus {
public:
    X64RegStatus() = default;
    X64Register Name;
    std::vector<X64Register> aliasName;
    X64RegStatus(X64Register name_,X64Register aliasname_ = NONE) {
        Name = name_;
        aliasName.push_back(aliasname_);
    }
    int Basewidth;
    IR::Operand occupyOperand;
    OperandContext * occupyOperandContext = nullptr;
    X64Register OccupyName;
    bool is_dirty = false; //callee save
    int StackOffset;    //存储在栈上的位置（rbp-StackOffset）

};

inline std::vector<X64RegStatus> genX64Reg() {
    std::vector<X64RegStatus> ret;
    ret.emplace_back(RAX, EAX);
    ret.emplace_back(RBX, EBX);
    ret.emplace_back(RCX, ECX);
    ret.emplace_back(RDX, EDX);
    ret.emplace_back(RSI, ESI);
    ret.emplace_back(RDI, EDI);
    ret.emplace_back(RBP, EBP);
    ret.emplace_back(RSP, ESP);
    ret.emplace_back(R8, R8D);
    ret.emplace_back(R9, R9D);
    ret.emplace_back(R10, R10D);
    ret.emplace_back(R11, R11D);
    ret.emplace_back(R12, R12D);
    ret.emplace_back(R13, R13D);
    ret.emplace_back(R14, R14D);
    ret.emplace_back(R15, R15D);

    ret.emplace_back(XMM0);
    ret.emplace_back(XMM1);
    ret.emplace_back(XMM2);
    ret.emplace_back(XMM3);
    ret.emplace_back(XMM4);
    ret.emplace_back(XMM5);
    ret.emplace_back(XMM6);
    ret.emplace_back(XMM7);
    ret.emplace_back(XMM8);
    ret.emplace_back(XMM9);
    ret.emplace_back(XMM10);
    ret.emplace_back(XMM11);
    ret.emplace_back(XMM12);
    ret.emplace_back(XMM13);
    ret.emplace_back(XMM14);
    ret.emplace_back(XMM15);

    ret.emplace_back(RIP);
    ret.emplace_back(FLAGS);

    return ret;
}

class BaseBlockContext {
    std::unordered_map<IR::Operand,OperandContext> IRlocMap;
    int rspOffset = 0; //当前rsp相比ret（rbp默认位置）的offset  //注意控制块中间的栈帧同步与spill问题
    std::vector<string> ASMcode;    //已经是最终输出，不进行格式化
};

} // namespace ASM



#endif