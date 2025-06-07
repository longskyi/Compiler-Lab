#include "asmGen.h"

namespace ASM {

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

inline auto x32Tox64(X64Register src) {
    switch (src) {
    case EAX: return RAX;
    case EBX: return RBX;
    case ECX: return RCX;
    case EDX: return RDX;
    case ESI: return RSI;
    case EDI: return RDI;
    case EBP: return RBP;
    case ESP: return RSP;
    case R8D: return R8;
    case R9D: return R9;
    case R10D: return R10;
    case R11D: return R11;
    case R12D: return R12;
    case R13D: return R13;
    case R14D: return R14;
    case R15D: return R15;
    default: return NONE;
    }
}

inline auto x64Tox32(X64Register src) {
    switch (src) {
    case RAX: return EAX;
    case RBX: return EBX;
    case RCX: return ECX;
    case RDX: return EDX;
    case RSI: return ESI;
    case RDI: return EDI;
    case RBP: return EBP;
    case RSP: return ESP;
    case R8: return R8D;
    case R9: return R9D;
    case R10: return R10D;
    case R11: return R11D;
    case R12: return R12D;
    case R13: return R13D;
    case R14: return R14D;
    case R15: return R15D;
    default: return NONE;
    }
}

//可能加上RBP
const std::unordered_set<X64Register> GPRs64 = {
    RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
    R8, R9, R10, R11, R12, R13, R14, R15
};

const std::unordered_set<X64Register> GPRs32 = {
    EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP,
    R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D
};

const std::unordered_set<X64Register> FPRsXMM = {
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15
};

inline string Eformat(X64Register reg) {
    switch (reg) {
        // 64-bit registers
        case RAX: return "rax";
        case RBX: return "rbx";
        case RCX: return "rcx";
        case RDX: return "rdx";
        case RSI: return "rsi";
        case RDI: return "rdi";
        case RBP: return "rbp";
        case RSP: return "rsp";
        case R8:  return "r8";
        case R9:  return "r9";
        case R10: return "r10";
        case R11: return "r11";
        case R12: return "r12";
        case R13: return "r13";
        case R14: return "r14";
        case R15: return "r15";

        // 32-bit registers
        case EAX:  return "eax";
        case EBX:  return "ebx";
        case ECX:  return "ecx";
        case EDX:  return "edx";
        case ESI:  return "esi";
        case EDI:  return "edi";
        case EBP:  return "ebp";
        case ESP:  return "esp";
        case R8D:  return "r8d";
        case R9D:  return "r9d";
        case R10D: return "r10d";
        case R11D: return "r11d";
        case R12D: return "r12d";
        case R13D: return "r13d";
        case R14D: return "r14d";
        case R15D: return "r15d";

        // XMM registers
        case XMM0:  return "xmm0";
        case XMM1:  return "xmm1";
        case XMM2:  return "xmm2";
        case XMM3:  return "xmm3";
        case XMM4:  return "xmm4";
        case XMM5:  return "xmm5";
        case XMM6:  return "xmm6";
        case XMM7:  return "xmm7";
        case XMM8:  return "xmm8";
        case XMM9:  return "xmm9";
        case XMM10: return "xmm10";
        case XMM11: return "xmm11";
        case XMM12: return "xmm12";
        case XMM13: return "xmm13";
        case XMM14: return "xmm14";
        case XMM15: return "xmm15";

        // Special registers
        case RIP:   return "rip";
        case FLAGS: return "flags";

        // Default case
        case NONE:
        default:
            return "";
    }
}

const std::unordered_set<X64Register> XMMREG = {XMM0,XMM1,XMM2,XMM3,XMM4,XMM5,XMM6,XMM7,XMM8,
XMM9,XMM10,XMM11,XMM12,XMM13,XMM14,XMM15};

struct OperandContext{
    // 针对alloca出来的特殊处理
    bool isStackPtr = false;    //可通过rbp+来计算的情况
    int rbpOffset;  //operand值可通过rsp/rbp计算
    //
    // spill出去了
    bool isSpill = false;
    int spillOffset;    //operand存储在栈上位置（rbp+StackOffset)
    //
    bool will_escape = false;    //生命周期逃离当前基本块
    size_t startIdx;    //从某条ir指令开始活跃
    std::vector<size_t> usedIdx;    //在某条ir指令被使用

    // not used 为未来更复杂寄存器分配算法预留
    std::unordered_set<X64Register> requireReg; //可使用寄存器
    std::unordered_set<X64Register> mustUseReg; //必须使用的寄存器
    //
};

class X64RegStatus {
public:
    X64RegStatus() = default;
    X64Register Name;
    std::vector<X64Register> aliasName;
    X64RegStatus(X64Register name_,X64Register aliasname_ = NONE,int basewidth = 64) {
        Name = name_;
        aliasName.push_back(aliasname_);
        Basewidth = basewidth;
    }
    int Basewidth = 64;
    IR::Operand occupyOperand;
    OperandContext * occupyOperandContext = nullptr;
    X64Register OccupyName;
    //callee save 
    bool is_dirty = false; //callee save
    int StackOffset;    //存储在栈上的位置（rbp-StackOffset）
    //callee save end
    inline bool is_Reg_valid(size_t InstIdx)const {
        if(is_dirty) return false;
        if(occupyOperand.empty()) return true;
        if(!occupyOperandContext) {
            std::cerr<<"寄存器分配错误，不可用的Operand Context\n";
            return false;
        }
        if(InstIdx > occupyOperandContext->usedIdx.back() && ! occupyOperandContext->will_escape) {
            return true;
        } else {
            return false;
        }
    }
    inline bool is_Reg_dirty() const {
        return  is_dirty;
    }
    inline void calleeSave(int stackOffset_) {
        if(!is_dirty) {
            std::cerr<<"callee save错误";
            return;
        }
        is_dirty = false;
        StackOffset = stackOffset_;
    }
    //为线性分配寄存器，方便排序使用
    inline size_t next_use_idx(size_t InstIdx) const {
        if(is_Reg_valid(InstIdx)) return 0;
        if(!occupyOperandContext) {
            std::cerr<<"寄存器分配错误，不可用的operand context\n";
        }
        auto & useVec = occupyOperandContext->usedIdx;
        auto it = std::upper_bound(useVec.begin(),useVec.end(),InstIdx);
        if(it == useVec.end()) {
            //应该不可能这么多inst
            if(occupyOperandContext->will_escape) return std::numeric_limits<int>::max()/2;
            return useVec.back();
        } 
        return *it;
    }
    inline bool is_reg_name(X64Register name) const {
        if(name == this->Name) return true;
        for(auto q : aliasName) {
            if(q == name) return true;
        }
        return false;
    }
    inline void occupy(X64Register usedName, IR::Operand & operand , OperandContext * OpContext) {
        this->occupyOperand = operand;
        this->occupyOperandContext = OpContext;
        this->OccupyName = usedName;
    }
    inline void clear() {
        this->occupyOperand.clear();
        this->occupyOperandContext = nullptr;
    }
    inline bool isXMM() const {
        return XMMREG.count(this->Name);
    }
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

    ret.emplace_back(XMM0,XMM0,128);
    ret.emplace_back(XMM1,XMM1,128);
    ret.emplace_back(XMM2,XMM2,128);
    ret.emplace_back(XMM3,XMM3,128);
    ret.emplace_back(XMM4,XMM4,128);
    ret.emplace_back(XMM5,XMM5,128);
    ret.emplace_back(XMM6,XMM6,128);
    ret.emplace_back(XMM7,XMM7,128);
    ret.emplace_back(XMM8,XMM8,128);
    ret.emplace_back(XMM9,XMM9,128);
    ret.emplace_back(XMM10,XMM10,128);
    ret.emplace_back(XMM11,XMM11,128);
    ret.emplace_back(XMM12,XMM12,128);
    ret.emplace_back(XMM13,XMM13,128);
    ret.emplace_back(XMM14,XMM14,128);
    ret.emplace_back(XMM15,XMM15,128);
    

    ret.emplace_back(RIP);
    ret.emplace_back(FLAGS);

    return ret;
}

inline std::vector<std::variant<X64Register,int64_t>> arrangeArgPlace(std::vector<IR::Operand> & args ){
    std::vector<std::variant<X64Register,int64_t>> ret;
    int intRegUsed = 0;
    int xmmRegUsed = 0;
    for(int i = 0 ; i < args.size() ; i++ ) {
        if(args[i].datatype == IR::Operand::i32) {
            switch (intRegUsed) {
            case 0:
                ret.push_back(EDI);
                break;
            case 1:
                ret.push_back(ESI);
                break;
            case 2:
                ret.push_back(EDX);
                break;
            case 3:
                ret.push_back(ECX);
                break;
            case 4:
                ret.push_back(R8D);
                break;
            case 5:
                ret.push_back(R9D);
                break;
            default:
                ret.push_back(0);
                break;
                std::cerr<<"错误 栈参数传值 not implement\n";
            }
            intRegUsed++;
        }
        else if(args[i].datatype == IR::Operand::f32) {
            switch (xmmRegUsed) {
            case 0:
                ret.push_back(XMM0);
                break;
            case 1:
                ret.push_back(XMM1);
                break;
            case 2:
                ret.push_back(XMM2);
                break;
            case 3:
                ret.push_back(XMM3);
                break;
            case 4:
                ret.push_back(XMM4);
                break;
            default:
                ret.push_back(0);
                break;
                std::cerr<<"错误 栈参数传值 not implement\n";
            }
            xmmRegUsed++;
        }
        else if(args[i].datatype == IR::Operand::ptr) {
            switch (intRegUsed) {
            case 0:
                ret.push_back(RDI);
                break;
            case 1:
                ret.push_back(RSI);
                break;
            case 2:
                ret.push_back(RDX);
                break;
            case 3:
                ret.push_back(RCX);
                break;
            case 4:
                ret.push_back(R8);
                break;
            case 5:
                ret.push_back(R9);
                break;
            default:
                ret.push_back(0);
                break;
                std::cerr<<"错误 栈参数传值 not implement\n";
            }
            intRegUsed++;
        } else {
            std::cerr<<"不支持的数据类型传参\n";
        }
    }
    return ret;
}

inline std::vector<std::variant<X64Register,int64_t>> arrangeArgPlace(IR::FunctionIR & funcIR) {
    auto & args = funcIR.Args;
    //是从左到右的顺序
    return arrangeArgPlace(funcIR.Args);
}


class BaseBlockContext { //Block上下文
public:
    inline static size_t next_id = 0; //唯一id方便使用
    std::vector<X64RegStatus> RegFile;  //排序有讲究，尽量把通用寄存器放前头
    std::unordered_map<IR::Operand,std::unique_ptr<OperandContext>> IRlocMap;
    int64_t rspOffset = 0; //当前rsp相比ret（rbp默认位置）的offset 一般总是为负 //注意控制块中间的栈帧同步与spill问题
    std::vector<string> dataSection;    //不含section头，只包含定义
    std::vector<string> ASMcode;    //已经是最终输出，不进行格式化
    inline BaseBlockContext() {
        RegFile = genX64Reg();
    }
    inline BaseBlockContext(const BaseBlockContext& other) 
        : rspOffset(other.rspOffset),
        dataSection(other.dataSection),  
        ASMcode(other.ASMcode) {         
        RegFile.reserve(other.RegFile.size());
        for (const auto& regStatus : other.RegFile) {
            RegFile.push_back(regStatus);  // 假设X64RegStatus是可平凡拷贝的
        }
        for (const auto& [operand, opCtx] : other.IRlocMap) {
            IRlocMap.emplace(
                operand,
                opCtx ? std::make_unique<OperandContext>(*opCtx) : nullptr 
            );
        }
    }
    void emit(const std::string & str) {
        ASMcode.emplace_back(str);
    }
    //按照控制流程图生成的时候，后继block从前继block继承时，需要保存的信息
    //rsp栈信息
    //regFile的信息，清理所有occupyOperand记录即可
    //IRlocMap中的alloca信息
    //其他需要删除
    void inherit_from(const BaseBlockContext & other) {
        this->rspOffset = other.rspOffset;
        this->RegFile = other.RegFile;
        for(auto & regf : RegFile) {
            regf.clear();
        }
        for(auto & locItem : other.IRlocMap) {
            auto  OperandCtx_ptr =  locItem.second.get();
            if(OperandCtx_ptr->isStackPtr) {    //alloca
                this->IRlocMap[locItem.first] = std::make_unique<OperandContext>(*OperandCtx_ptr);
            }
        }
    }
    //为了进行同步，需要进行以下操作
    //修改rsp栈帧
    //同步所有dirty（如果源是dirty，后继不是dirty，则需要处理（且与源分配相同））
    //phi内容，暂不支持
    std::vector<std::string> sync_to(const BaseBlockContext & other) {
        std::vector<std::string> retASM;
        const std::unordered_set<X64Register> calleeSaveRegs = {RBX, R12, R13, R14, R15, RBP};

        // 1. 同步栈帧 - 调整rsp到相同位置
        if (this->rspOffset != other.rspOffset) {
            int64_t offsetDiff = other.rspOffset - this->rspOffset;
            if (offsetDiff > 0) {
                retASM.push_back("sub rsp, " + std::to_string(offsetDiff));
            } else {
                retASM.push_back("add rsp, " + std::to_string(-offsetDiff));
            }
        }

        // 2. 同步callee-save寄存器
        for (size_t i = 0; i < this->RegFile.size(); ++i) {
            const auto& thisReg = this->RegFile[i];
            const auto& otherReg = other.RegFile[i];

            // 只处理callee-save寄存器
            if (!calleeSaveRegs.count(thisReg.Name)) continue;

            // 情况1: 当前是dirty而目标不是dirty - 需要存储到栈
            if (thisReg.is_dirty && !otherReg.is_dirty) {
                int64_t offRbp = otherReg.StackOffset; // 使用目标的栈位置
                int64_t off = offRbp - other.rspOffset; // 注意此时，本地的栈已经提前修改了
                
                std::string movInstr = thisReg.isXMM() ? 
                    ((off % 16 == 0) ? "movaps" : "movups") : "mov";
                
                retASM.push_back(movInstr + " [rsp" + 
                            (off >= 0 ? "+" : "") + std::to_string(off) + 
                            "], " + Eformat(thisReg.Name));
            }
            // 情况2: 当前不是dirty而目标是dirty - 需要从栈恢复
            else if (!thisReg.is_dirty && otherReg.is_dirty) {
                int64_t offRbp = otherReg.StackOffset;
                int64_t off = offRbp - other.rspOffset;
                
                std::string movInstr = thisReg.isXMM() ? 
                    ((off % 16 == 0) ? "movaps" : "movups") : "mov";
                
                retASM.push_back(movInstr + " " + Eformat(thisReg.Name) + 
                            ", [rsp" + (off >= 0 ? "+" : "") + 
                            std::to_string(off) + "]");
            }
        }

        return retASM;
    }
    //输入对于理论rbp的offset，输出此时相对rsp的offset
    // target = rbp + rbp_off
    // rsp = rbp + rspOffset
    // target = rsp + x
    //rbp + rspOffset + x = rbp + rbp_off
    // x = rbp_off - rspOffset
    inline int64_t culRspOff(int64_t rbp_off) {
        return rbp_off - rspOffset;
    }
    //强制声明某个寄存器现在存储了某个operand
    void DeclareOcuupy(X64Register dst,IR::Operand op) {
        for(auto & regf:RegFile) {
            if(regf.is_reg_name(dst)) {
                regf.occupy(dst,op,IRlocMap.at(op).get());
            }
        }
    }
    void ClearOcuupy(X64Register dst) {
        for(auto & regf:RegFile) {
            if(regf.is_reg_name(dst)) {
                regf.clear();
            }
        }
    }
    void spill(X64Register src , size_t InstIdx) {
        for(auto & regf : RegFile) {
            if(regf.is_reg_name(src)) {
                spill(regf,InstIdx);
            }
        }
    }
    void spill(X64RegStatus& src, size_t InstIdx) {
        const size_t reg_size = src.Basewidth / 8;
        // 对齐要求（仅XMM需要16字节对齐）
        int64_t adjust = 0;
        if (src.isXMM()) {
            int64_t new_rsp = rspOffset - reg_size;
            int64_t misalignment = (-new_rsp) % 16;
            if (misalignment != 0) {
                adjust = 16 - misalignment;
            }
        }

        if (adjust > 0 || reg_size > 0) {
            int64_t total_sub = adjust + reg_size;
            ASMcode.push_back("sub rsp, " + std::to_string(total_sub));
            rspOffset -= total_sub;
        }
        // 3. 生成spill指令（总是使用[rsp+0]，因为总是在栈顶）
        if (src.isXMM()) {
            // 由于已经处理对齐，这里可以直接用movaps
            ASMcode.push_back("movaps [rsp], " + Eformat(src.Name)); 
        } else {
            ASMcode.push_back("mov [rsp], " + Eformat(src.Name));
        }
        
        if(src.is_Reg_dirty()) {
            src.is_dirty = false;
            src.StackOffset = rspOffset;
        } else if(!src.is_Reg_valid(InstIdx)) {
            src.occupyOperandContext->isSpill = true;
            src.occupyOperandContext->spillOffset = rspOffset;
            src.clear();
        }
    }
    //target是目标寄存器，contrains是要求不更改这些寄存器，返回实际分配到的寄存器
    X64Register allocReg(const std::unordered_set<X64Register> & target,const std::unordered_set<X64Register> & contrains , size_t InstIdx) {
        //分为三道pass，第一道pass查看有没有空的，有的话则返回
        //第二道pass，如果第一道pass失败，则尝试使用dirty的reg
        //第三道pass，如果还是失败，则使用最后一次使用最晚的，spill出去并返回

        auto matchRequire = [](const std::unordered_set<X64Register> & target,const std::unordered_set<X64Register> & contrains,X64RegStatus & regStatus) {
            for(const auto & tg : target) {
                if(regStatus.is_reg_name(tg)) {
                    bool allowed = true;
                    for(const auto & constrain : contrains) {
                        if(regStatus.is_reg_name(constrain)) {
                            allowed = false;
                            break;
                        }
                    }
                    if(allowed) return tg;
                    else return NONE;
                }
            }
            return NONE;
        };
        for (auto& regStatus : RegFile) {
            auto ret = matchRequire(target, contrains, regStatus);
            if(ret != NONE) {
                if(regStatus.is_Reg_valid(InstIdx)) return ret;
            }
        }
        for (auto& regStatus : RegFile) {
            auto ret = matchRequire(target, contrains, regStatus);
            if (ret!=NONE && regStatus.is_Reg_dirty()) {
                //脏寄存器需要spill出去
                spill(regStatus, InstIdx);
                return ret;
            }
        }

        X64RegStatus* farthestReg = nullptr;
        size_t farthestUse = 0;
        
        for (auto& regStatus : RegFile) {
            auto ret = matchRequire(target, contrains, regStatus);
            if (ret != NONE) {
                size_t nextUse = regStatus.next_use_idx(InstIdx);
                if (nextUse > farthestUse) {
                    farthestUse = nextUse;
                    farthestReg = &regStatus;
                }
            }
        }
        if (farthestReg) {
            spill(*farthestReg, InstIdx);
            return matchRequire(target, contrains, *farthestReg);
        }
        //失败
        std::cerr<<"寄存器分配失败\n";
        for (auto& regStatus : RegFile) {
            if (target.count(regStatus.Name) && regStatus.is_Reg_valid(InstIdx)) {
                return regStatus.Name;
            }
        }
        return NONE;
    }

    X64Register MoveOperandToTargetReg(const std::unordered_set<X64Register> & target,IR::Operand & src,size_t InstIdx,const std::unordered_set<X64Register> & contrains ={}) {
        //首先检查目标操作数在不在target里，如果在，直接返回
        //如果不在，确定操作数类型，并给出操作
        X64Register srcReg = NONE;
        for(auto & reg:RegFile){
            if(reg.occupyOperand == src) {
                srcReg = reg.OccupyName;
                for(auto & regN : target ){
                    if(reg.is_reg_name(regN)) {
                        return regN;
                    }
                }
            }
        }

        // 从预处理Map中获取操作数的位置信息
        auto it = IRlocMap.find(src);
        if (it == IRlocMap.end()) {
            std::cerr << "内部错误：找不到操作数的位置信息\n";
            return NONE;
        }
        auto& loc = *it->second;

        // 分配目标寄存器
        X64Register targetReg = allocReg(target, contrains, InstIdx);
        if (targetReg == NONE) {
            std::cerr << "错误：无法分配目标寄存器\n";
            return NONE;
        }

        // 根据操作数类型和位置生成相应的加载代码
        if (src.datatype == IR::Operand::LABEL) {
            std::cerr << "内部错误，尝试将label送入寄存器\n";
            return NONE;
        }

        // 处理立即数
        if (src.memplace == IR::Operand::IMM) {
            if (src.datatype == IR::Operand::OperandType::i32) {
                // 32位整数立即数加载到通用寄存器
                ASMcode.push_back("mov " + Eformat(targetReg) + ", " + std::to_string(src.valueI));
            }
            else if (src.datatype == IR::Operand::OperandType::f32) {
                // 32位浮点立即数需要先在数据段定义(x64限制)
                string label = "float_const_" + std::to_string(next_id++);
                dataSection.push_back(label + ": .float " + std::to_string(src.valueF));
                ASMcode.push_back("movss " + Eformat(targetReg) + ", " + label + "[rip]");
            }
            else if (src.datatype == IR::Operand::OperandType::ptr) {
                // 64位指针立即数
                ASMcode.push_back("mov " + Eformat(targetReg) + ", " + std::to_string(src.valueI));
            }
        }

        // 处理栈指针偏移（alloca 情况）
        else if (loc.isStackPtr) {
            int64_t offRbp = loc.rbpOffset;   // rbp + offRbp
            int64_t off = offRbp-rspOffset;     // rsp = rbp - rspOffset
            // 生成 lea 指令来计算地址
            if (off == 0) {
                ASMcode.push_back("lea " + Eformat(targetReg) + ", [rsp]");
            } else {
                ASMcode.push_back("lea " + Eformat(targetReg) + ", [rsp" + 
                                (off > 0 ? "+" : "") + std::to_string(off) + "]");
            }
        }
        // 处理spill数据
        else if (loc.isSpill) {
            int64_t offRbp = loc.spillOffset;
            int64_t off = offRbp-rspOffset;
            
            if (src.datatype == IR::Operand::OperandType::i32) {
                ASMcode.push_back("mov " + Eformat(targetReg) + ", [rsp" + 
                                (off >= 0 ? "+" : "") + std::to_string(off) + "]");
            }
            else if (src.datatype == IR::Operand::OperandType::f32) {
                ASMcode.push_back("movss " + Eformat(targetReg) + ", [rsp" + 
                                (off >= 0 ? "+" : "") + std::to_string(off) + "]");
            }
            else if (src.datatype == IR::Operand::OperandType::ptr) {
                ASMcode.push_back("mov " + Eformat(targetReg) + ", [rsp" + 
                                (off >= 0 ? "+" : "") + std::to_string(off) + "]");
            }
        }
        // 处理已经在寄存器中的操作数
        else if (srcReg != NONE) {
            // 需要寄存器到寄存器的移动
            if (XMMREG.count(targetReg) && XMMREG.count(srcReg)) {
                ASMcode.push_back("movaps " + Eformat(targetReg) + ", " + Eformat(srcReg));
            }
            else if (!XMMREG.count(targetReg) && !XMMREG.count(srcReg)) {
                ASMcode.push_back("mov " + Eformat(targetReg) + ", " + Eformat(srcReg));
            }
            else {
                // 寄存器类型不匹配，需要特殊处理
                std::cerr << "警告：寄存器类型不匹配的移动\n";
                return NONE;
            }
        // } else if(src.memplace == IR::Operand::GLOBAL && src.datatype == IR::Operand::i32) {
        //     //从data section中取数据
        //     emit(std::format("lea {} , {}[rip]",Eformat(targetReg),toString_view(src.LiteralName)));
        } else if(src.memplace == IR::Operand::GLOBAL && src.datatype == IR::Operand::ptr) {
            //从data section中取数据
            //这里global留了一个坑，这么做是不能使用extern的
            emit(std::format("lea {} , {}[rip]",Eformat(targetReg),toString_view(src.LiteralName)));
            
            //emit(std::format("mov {} ,[{}]",Eformat(targetReg),Eformat(targetReg)));
        }
        // 更新寄存器状态
        for (auto& reg : RegFile) {
            if (reg.is_reg_name(targetReg)) {
                reg.occupy(targetReg, src,IRlocMap.at(src).get());
                reg.occupyOperandContext = &loc;
                break;
            }
        }

        return targetReg;
    }
    //主要进行两件事，一个是找到最大的alloca offset，调整rsp到那个位置
    //一个是将arg塞进register，同时设置dirty register
    void InitArgsWithFunctionIR(IR::FunctionIR & funcIR) {
        //funcIR.
        int64_t maxSize = 0;
        for(auto & inst : funcIR.Instblock) {
            if(std::holds_alternative<IR::allocaOpInst>(inst)) {
                auto aloc = std::get<IR::allocaOpInst>(inst);
                if(aloc.offset>maxSize) maxSize = aloc.offset;
                this->IRlocMap[aloc.ret]->isStackPtr = true;
                this->IRlocMap[aloc.ret]->rbpOffset = - aloc.offset;    //负数
            }
        } 
        if(maxSize > 0 ) {
            //分配栈空间
            ASMcode.push_back("sub rsp, " + std::to_string(maxSize+8)); // +8使rsp基地址对齐
            rspOffset = - maxSize;
        }
        //设置dirty
        const std::unordered_set<X64Register> resotreReg = {RBX,R12,R13,R14,R15,RBP};
        for(auto & regf : RegFile) {
            if(resotreReg.count(regf.Name)) {
                regf.is_dirty = true;
                regf.occupyOperand.clear();
            }
        }
        //给输入参数安排寄存器位置 
        auto InitPlace = arrangeArgPlace(funcIR);
        for(int i =0 ; i < InitPlace.size() ; i++) {
            auto & place = InitPlace[i];
            if(std::holds_alternative<X64Register>(place)) {
                auto regName = std::get<X64Register>(place);
                for(auto & regf : RegFile) {
                    if(regf.is_reg_name(regName)) {
                        regf.occupy(regName,funcIR.Args[i],IRlocMap.at(funcIR.Args[i]).get());
                    }
                }
            }
        }
        return;
    }
    void restoreCalleeSave(const std::unordered_set<X64Register> regs) {
        for(auto & regf: this->RegFile) {
            for(auto & regName : regs) {
                if(regf.is_reg_name(regName) && !regf.is_dirty) {
                    int64_t offRbp = regf.StackOffset;   // rbp + offRbp
                    int64_t off = offRbp-rspOffset;     // rsp = rbp - rspOffset
                    ASMcode.push_back("mov " + Eformat(regName) + ", [rsp" + 
                                (off >= 0 ? "+" : "") + std::to_string(off) + "]");
                    regf.is_dirty = true;
                    regf.clear();
                }
            }
        }
    }
    void RETrestore() {
        const std::unordered_set<X64Register> resotreReg = {RBX,R12,R13,R14,R15,RBP};
        //恢复寄存器
        restoreCalleeSave(resotreReg);
        //恢复栈帧
        //size_t off = (-this->rspOffset)-8;
        size_t off = (-this->rspOffset) + 8;    //包括8字节空，毗邻预留ret地址
        if(off>0)
            ASMcode.push_back("add "+ Eformat(RSP)+", "+std::to_string(off));
    }
    void InitOperandWithIRBaseBlock(IR::IRBaseBlock * baseBlock) {
        auto markDef = [&](IR::Operand & op,size_t idx){
            if(this->IRlocMap.count(op)) {
                std::cerr<<"不满足SSA\n";
                return;
            }
            this->IRlocMap[op] = std::make_unique<OperandContext>();
            this->IRlocMap[op]->startIdx = idx;
        };
        auto markUse = [&](IR::Operand & op,size_t idx){
            if(!this->IRlocMap.count(op)) {
                this->IRlocMap[op] = std::make_unique<OperandContext>();
                this->IRlocMap[op]->startIdx = idx;
            }
            this->IRlocMap[op]->usedIdx.push_back(idx);
        };
        //给所有operand设置 起始和使用 
        auto instBlock = baseBlock->Insts;
        for(size_t instIdx = 0 ; instIdx < instBlock.size() ; instIdx++) {
            auto & inst = instBlock[instIdx];
            std::visit(overload {
                [&](IR::BinaryOpInst& op) {
                    markDef(op.dst, instIdx);
                    if(op.op == IR::BinaryOpInst::add) {
                        markUse(op.src1, instIdx);
                        markUse(op.src2, instIdx);
                    } else if(op.op == IR::BinaryOpInst::mul) {
                        markUse(op.src1, instIdx);
                        markUse(op.src2, instIdx);
                    } else if(op.op == IR::BinaryOpInst::cast) {
                        markUse(op.src1, instIdx);
                    } else if(op.op == IR::BinaryOpInst::cmp) {
                        markUse(op.src1, instIdx);
                        markUse(op.src2, instIdx);
                    } else if(op.op == IR::BinaryOpInst::COPY) {
                        markUse(op.src1, instIdx);
                    }
                    return true;
                },
                [&](IR::callOpInst& op) {
                    if(!op.ret.empty()) {
                        markDef(op.ret, instIdx);
                    }
                    for(auto & ops : op.srcs) {
                        markUse(ops, instIdx);
                    }
                    markUse(op.targetFUNC, instIdx);
                    return true;
        
                },
                [&](IR::allocaOpInst& op) {
                    markDef(op.ret, instIdx);
                    return true;
                },
                [&](IR::memOpInst& op) {
                    if(op.memOP == IR::memOpInst::load) {
                        markDef(op.dst, instIdx);
                        markUse(op.src, instIdx);
                    } else if(op.memOP == IR::memOpInst::store) {
                        markUse(op.dst, instIdx);
                        markUse(op.src, instIdx);
                    }
                    return true;
                    
                },
                [&](IR::retInst& op) {
                    if(!op.src.empty()) {
                        markUse(op.src, instIdx);
                    }
                    return true;
                    
                },
                [&](IR::printInst& op) {
                    if(!op.src.empty()) {
                        markUse(op.src, instIdx);
                    }
                    return true;
                    
                },
                [&](IR::BrInst& op) {
                    return true;
                    
                },
                [&](IR::phiInst& op) {
                    return false;
                    
                },
                [&](IR::labelInst& op) {
                    return true;            
                }
            }, inst);
        }
    }
};



std::unordered_set<X64Register> inferRegisterType(const IR::Operand & src) {
    if(src.datatype== IR::Operand::LABEL) {
        std::cerr<<"错误，尝试对label获取类型\n";
        return {};
    }
    if(src.datatype == IR::Operand::i32) {
        return GPRs32;
    }
    if(src.datatype == IR::Operand::ptr) {
        return GPRs64;
    }
    if(src.datatype == IR::Operand::f32) {
        return FPRsXMM;
    }
    std::cerr<<"错误，不支持的类型infer\n";
    return {};
}

class FuncASMGenerator {
private:
    struct BlockGenInfo {
        BlockGenInfo * inherit_from = nullptr;
        std::unique_ptr<BaseBlockContext> initialCtx = nullptr;  // 该块的初始上下文
        std::unique_ptr<BaseBlockContext> exitCtx = nullptr;     // 该块执行结束后的上下文
        std::vector<std::string> asmCode;              // 该块生成的汇编
        bool generated = false;                        // 是否已生成
    };
    std::unordered_map<IR::IRBaseBlock *,BlockGenInfo> BlockInfoMap;
    std::vector<IR::IRBaseBlock *> GenSequence;

public:
    std::vector<string> globalDataSection;
    std::vector<string> codeSection;
    inline bool generateFunction(IR::FunctionIR& func) {
        auto & BlockLists = func.IRoptimized->BlockLists;
        auto start = func.IRoptimized->enterBlock;
        std::unordered_set<IR::IRBaseBlock *>visited;
        std::function<void(IR::IRBaseBlock *,IR::IRBaseBlock*)> DFSgenSequence = [&](IR::IRBaseBlock * curr,IR::IRBaseBlock * inherit_from){
            if(!curr) return;
            if(visited.count(curr)) {
                return ;
            }
            visited.insert(curr);
            GenSequence.push_back(curr);
            if(inherit_from)
                BlockInfoMap[curr].inherit_from = &BlockInfoMap[inherit_from];
            for(auto & successor : curr->successor) {
                DFSgenSequence(successor,curr);
            }
        };
        //确定生成asm的顺序
        DFSgenSequence(start,nullptr);
        
        BlockInfoMap[start].initialCtx = std::make_unique<BaseBlockContext>();
        //调用约定初始化
        BlockInfoMap[start].initialCtx->InitOperandWithIRBaseBlock(start);
        BlockInfoMap[start].initialCtx->InitArgsWithFunctionIR(func);
        //BlockInfoMap[start].initialCtx->rspOffset += -8; //call之前16字节对齐，call之后压入了8字节导致不对齐，需要记录
        for(auto irblock : GenSequence) {
            if(BlockInfoMap[irblock].initialCtx == nullptr) {
                BlockInfoMap[irblock].initialCtx = std::make_unique<BaseBlockContext>();
                BlockInfoMap[irblock].initialCtx->inherit_from(*BlockInfoMap[irblock].inherit_from->exitCtx);
            }
            BlockInfoMap[irblock].exitCtx = std::make_unique<BaseBlockContext>(*BlockInfoMap[irblock].initialCtx);
            if(irblock != start) {
                BlockInfoMap[irblock].exitCtx->InitOperandWithIRBaseBlock(irblock);
            }
            if(!BaseBlockASMGen(*BlockInfoMap[irblock].exitCtx,*irblock)) return false;
            BlockInfoMap[irblock].generated = true;
            //打印生成的asm
            // for(auto & s : BlockInfoMap[irblock].exitCtx->ASMcode) {
            //     std::cout<<s<<std::endl;
            // }
            //std::cout<<"funcBaseBlock END\n";
            BlockInfoMap[irblock].asmCode = BlockInfoMap[irblock].exitCtx->ASMcode;
            globalDataSection.insert(globalDataSection.end(),BlockInfoMap[irblock].exitCtx->dataSection.begin(),BlockInfoMap[irblock].exitCtx->dataSection.end());            //end
        }
        for(auto blockPtr : GenSequence) {
            codeSection.insert(codeSection.end(),BlockInfoMap[blockPtr].asmCode.begin(),BlockInfoMap[blockPtr].asmCode.end());
        }
        return true;
    }
    bool BaseBlockASMGen(BaseBlockContext & BlockCtx ,IR::IRBaseBlock & IRblock);
        
};


bool X64ASMGenerator::genASM(IR::IRContent & IRs,std::ostream & out) {
        std::vector<string> Rdatasection;
        out<<".intel_syntax noprefix\n";
        out<<".section .data\n";
        out<<".align 8\n";
        for(auto & globalVar : IRs.globalVar) {
            if(globalVar.size == 4)
                out<<std::format("\t{}: .long {}\n",toString_view(globalVar.name),globalVar.initValue);
            else
                out<<std::format("\t{}: .zero {}\n",toString_view(globalVar.name),globalVar.size);
            
        }
        out<<".section .text\n";
        for(auto & func : IRs.FunctionIR) {
            FuncASMGenerator funcgen;
            out<<std::format("\t.global {}\n",toString_view(func.funcName));
            out<<std::format("{}:\n",toString_view(func.funcName));
            if(!funcgen.generateFunction(func)) {
                std::cerr<<"生成失败\n";
                return false;
            }
            Rdatasection.insert(Rdatasection.end(),funcgen.globalDataSection.begin(),funcgen.globalDataSection.end());
            for(auto & inst : funcgen.codeSection) {
                out<<"\t"<<inst<<"\n";
            }
        }
        out<<".section .rodata\n";
        for(auto & LC : Rdatasection) {
            out<<"\t"<<LC<<"\n";
        }
        return true;
}


bool BinaryOpASMGen(IR::BinaryOpInst& op, size_t InstIdx ,FuncASMGenerator & ASMGen,BaseBlockContext & BlockCtx ,IR::IRBaseBlock & IRblock) {
    auto checkLastUse = [&](IR::Operand & op){
        return InstIdx >= BlockCtx.IRlocMap[op]->usedIdx.back();
    };
    if(op.op == IR::BinaryOpInst::add) {
        if(op.op_type == IR::BinaryOpInst::i32 || op.op_type == IR::BinaryOpInst::ptr) {
            // add rax , rbx 会覆盖rax
            // add rax , num 支持数字
            // 如果第一个寄存器是最后一次使用（说明可以覆盖）
            auto src1Regs = inferRegisterType(op.src1);
            auto reg1 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src1,InstIdx);
            // src1已经移动进reg1
            if(!checkLastUse(op.src1) && !BlockCtx.IRlocMap[op.src1]->isStackPtr) {
                //非最后一次使用，spill，寄存器不会清空
                //如果src本身是stack指针，也不spill了
                BlockCtx.spill(reg1,InstIdx);
            }
            //非最后一次使用，spill 寄存器不会清空
            if(op.src2.memplace == IR::Operand::IMM) {
                //支持立即数  文法中，立即数不会出现64位，所以可以安全用add reg1 立即数
                BlockCtx.ASMcode.push_back("add "+Eformat(reg1)+", "+std::to_string(op.src2.valueI));
            }
            else {
                src1Regs.erase(reg1);
                auto reg2 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src2,InstIdx,{reg1});
                BlockCtx.ASMcode.push_back("add "+Eformat(reg1)+", "+ Eformat(reg2));
            }
            //声明reg1被目标寄存器占领
            BlockCtx.DeclareOcuupy(reg1,op.dst);
        }
        if(op.op_type == IR::BinaryOpInst::f32) {
            // addss xmm0 , xmm1 会覆盖xmm0 不支持数字
            // 如果第一个寄存器是最后一次使用（说明可以覆盖）
            auto src1Regs = inferRegisterType(op.src1);
            auto reg1 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src1,InstIdx);
            // src1已经移动进reg1
            if(!checkLastUse(op.src1)) {
                //非最后一次使用，spill，寄存器不会清空
                //尽管spill不是最佳选择，但是还是选了
                BlockCtx.spill(reg1,InstIdx);
            }
            src1Regs.erase(reg1);
            auto reg2 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src2,InstIdx,{reg1});
            BlockCtx.ASMcode.push_back("addss "+Eformat(reg1)+", "+ Eformat(reg2));
            //声明reg1被目标寄存器占领
            BlockCtx.DeclareOcuupy(reg1,op.dst);
        }
    } else if(op.op == IR::BinaryOpInst::mul) {
        if(op.op_type == IR::BinaryOpInst::i32 || op.op_type == IR::BinaryOpInst::ptr) {
            // imul rax , rbx 会覆盖rax，和rdx
            // imul rax , num 支持数字
            // 如果第一个寄存器是最后一次使用（说明可以覆盖）
            X64Register dxConstrain;
            if(op.op_type == IR::BinaryOpInst::i32) {
                dxConstrain = EDX;
            }
            else{
                dxConstrain = RDX;
            }
            BlockCtx.allocReg({dxConstrain},{},InstIdx);
            auto src1Regs = inferRegisterType(op.src1);
            src1Regs.erase(dxConstrain);
            auto reg1 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src1,InstIdx,{dxConstrain});
            // src1已经移动进reg1
            if(!checkLastUse(op.src1) && !BlockCtx.IRlocMap[op.src1]->isStackPtr) {
                //非最后一次使用，spill，寄存器不会清空
                //如果src本身是stack指针，也不spill了
                BlockCtx.spill(reg1,InstIdx);
            }
            //非最后一次使用，spill 寄存器不会清空
            if(op.src2.memplace == IR::Operand::IMM) {
                //支持立即数  文法中，立即数不会出现64位，所以可以安全用add reg1 立即数
                BlockCtx.ASMcode.push_back("imul "+Eformat(reg1)+", "+std::to_string(op.src2.valueI));
            }
            else {
                src1Regs.erase(reg1);
                auto reg2 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src2,InstIdx,{reg1,dxConstrain});
                BlockCtx.ASMcode.push_back("imul "+Eformat(reg1)+", "+ Eformat(reg2));
            }
            //声明reg1被目标寄存器占领
            BlockCtx.DeclareOcuupy(reg1,op.dst);
        }
        if(op.op_type == IR::BinaryOpInst::f32) {
            // mulss xmm0 , xmm1 会覆盖xmm0 不支持数字
            // 如果第一个寄存器是最后一次使用（说明可以覆盖）
            auto src1Regs = inferRegisterType(op.src1);
            auto reg1 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src1,InstIdx);
            // src1已经移动进reg1
            if(!checkLastUse(op.src1)) {
                //非最后一次使用，spill，寄存器不会清空
                //尽管spill不是最佳选择，但是还是选了
                BlockCtx.spill(reg1,InstIdx);
            }
            src1Regs.erase(reg1);
            auto reg2 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src2,InstIdx,{reg1});
            BlockCtx.ASMcode.push_back("mulss "+Eformat(reg1)+", "+ Eformat(reg2));
            //声明reg1被目标寄存器占领
            BlockCtx.DeclareOcuupy(reg1,op.dst);
        }
    } else if(op.op == IR::BinaryOpInst::cast) {
        if(op.op_type == IR::BinaryOpInst::i32Tof32) {
            auto srcRegs = inferRegisterType(op.src1);
            auto dstRegs = inferRegisterType(op.dst);
            auto srcReg = BlockCtx.MoveOperandToTargetReg(srcRegs,op.src1,InstIdx,{});
            auto dstReg = BlockCtx.allocReg(dstRegs,{srcReg},InstIdx);
            BlockCtx.emit(std::format("cvsi2ss {}, {}",Eformat(dstReg),Eformat(srcReg)));
            BlockCtx.DeclareOcuupy(dstReg,op.dst);
        }
        if(op.op_type == IR::BinaryOpInst::f32Toi32) {
            auto srcRegs = inferRegisterType(op.src1);
            auto dstRegs = inferRegisterType(op.dst);
            auto srcReg = BlockCtx.MoveOperandToTargetReg(srcRegs,op.src1,InstIdx,{});
            auto dstReg = BlockCtx.allocReg(dstRegs,{srcReg},InstIdx);
            BlockCtx.emit(std::format("cvttss2si {}, {}",Eformat(dstReg),Eformat(srcReg)));
            BlockCtx.DeclareOcuupy(dstReg,op.dst);
        }
        if(op.op_type == IR::BinaryOpInst::i32Toptr) {
            auto srcRegs = inferRegisterType(op.src1);
            auto reg1 = BlockCtx.MoveOperandToTargetReg(srcRegs,op.src1,InstIdx,{});
            if(!checkLastUse(op.src1)) {
                //非最后一次使用，spill，寄存器不会清空
                //尽管spill不是最佳选择，但是还是选了
                BlockCtx.spill(reg1,InstIdx);
            }
            BlockCtx.DeclareOcuupy(x32Tox64(reg1),op.dst);
        }
        if(op.op_type == IR::BinaryOpInst::ptrToi32) {
            auto srcRegs = inferRegisterType(op.src1);
            auto reg1 = BlockCtx.MoveOperandToTargetReg(srcRegs,op.src1,InstIdx,{});
            if(!checkLastUse(op.src1)) {
                //非最后一次使用，spill，寄存器不会清空
                //尽管spill不是最佳选择，但是还是选了
                BlockCtx.spill(reg1,InstIdx);
            }
            BlockCtx.DeclareOcuupy(x64Tox32(reg1),op.dst);
        }
    } else if(op.op == IR::BinaryOpInst::cmp) {
        //下一条一定是br
        auto src1Regs = inferRegisterType(op.src1);
        auto reg1 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src1,InstIdx);
        src1Regs.erase(reg1);
        auto reg2 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src2,InstIdx,{reg1});
        if(op.src1.datatype == IR::Operand::f32) {
            //不考虑浮点触发异常情况
            BlockCtx.emit(std::format("comiss {}, {}",Eformat(reg1),Eformat(reg2)));
        } else {
            BlockCtx.emit(std::format("cmp {}, {}",Eformat(reg1),Eformat(reg2)));
        }
        
    } else if(op.op == IR::BinaryOpInst::COPY) {
        auto src1Regs = inferRegisterType(op.src1);
        auto reg1 = BlockCtx.MoveOperandToTargetReg(src1Regs,op.src1,InstIdx);
        src1Regs.erase(reg1);

        auto reg2 = BlockCtx.allocReg(src1Regs,{reg1},InstIdx);
        BlockCtx.DeclareOcuupy(reg2,op.dst);
        if(op.src1.datatype == IR::Operand::f32) {
            BlockCtx.emit(std::format("mov {}, {}",Eformat(reg2),Eformat(reg1)));
        } else {
            //假定reg2已经高位置0
            BlockCtx.emit(std::format("movaps {}, {}",Eformat(reg2),Eformat(reg1)));
        }
    }
    else {
        std::cerr<<"不支持的binary OP\n";
        return false;
    }
    return true;
}


bool memOpASMGen(IR::memOpInst& op, size_t InstIdx ,FuncASMGenerator & ASMGen,BaseBlockContext & BlockCtx ,IR::IRBaseBlock & IRblock) {
    if(op.memOP == IR::memOpInst::load) {
        //mov dst , src
        auto dstregS = inferRegisterType(op.dst);
        auto reg2 = BlockCtx.allocReg(dstregS,{},InstIdx);
        BlockCtx.DeclareOcuupy(reg2,op.dst);
        std::string movType="mov ";
        if(op.dst.datatype == IR::Operand::f32) {
            movType = "movss ";
        }
        if(BlockCtx.IRlocMap[op.src]->isStackPtr) {
            //不用分配reg1
            int64_t off = BlockCtx.culRspOff(BlockCtx.IRlocMap[op.src]->rbpOffset);
            BlockCtx.ASMcode.push_back(movType + Eformat(reg2)+", [rsp"+
            (off >= 0 ? "+" : "") + std::to_string(off) + "]");
        }
        else {
            auto srcregS = inferRegisterType(op.src);
            auto reg1 = BlockCtx.MoveOperandToTargetReg(srcregS,op.src,InstIdx,{reg2});
            BlockCtx.ASMcode.push_back(movType+Eformat(reg2)+", ["+Eformat(reg1)+"]");
        }
    } else if(op.memOP == IR::memOpInst::store) {
        //mov src dst
        std::string movType="mov ";
        if(op.src.datatype == IR::Operand::f32) {
            movType = "movss ";
        }
        auto srcregS = inferRegisterType(op.src);
        auto reg2 = BlockCtx.MoveOperandToTargetReg(srcregS,op.src,InstIdx,{});
        if(BlockCtx.IRlocMap[op.dst]->isStackPtr) {
            //不用分配reg1
            int64_t off = BlockCtx.culRspOff(BlockCtx.IRlocMap[op.dst]->rbpOffset);
            BlockCtx.ASMcode.push_back(movType+"[rsp" + std::string("") +
            (off >= 0 ? "+" : "") + std::to_string(off) + "]" +", " + Eformat(reg2));
        }
        else {
            auto dstregS = inferRegisterType(op.dst);
            auto reg1 = BlockCtx.MoveOperandToTargetReg(dstregS,op.dst,InstIdx,{reg2});
            BlockCtx.ASMcode.push_back(movType+"["+Eformat(reg1)+"], "+Eformat(reg2));
        }
    }
    return true;
}

bool printASMGen(IR::printInst& op, size_t InstIdx ,BaseBlockContext & BlockCtx) {
    BlockCtx.MoveOperandToTargetReg({EAX},op.src,InstIdx);
    size_t id[5];
    for(int i =0 ; i<5;i++) {
        id[i] = BaseBlockContext::next_id++;    
    }
    
    std::vector<std::string> assembly = {
        //分配 ebx存储原始值，ecx存储字符数，rsi指向起始地址
        "push   rax",
        "push   rbx",
        "push   rcx",
        "push   rdx",
        "push   rdi",
        "push   rsi",
        "push   rbp",
        "mov    ebx, eax",           // 保存原始值到ebx
        "xor    ecx, ecx",           // ecx = 数字位数计数器

        "sub    rsp , 1",
        "mov byte ptr [rsp] , 0x0A",    //添加换行符
        "inc    ecx",

        "test   eax, eax",
        std::format("jns    positive_label{}",id[0]),
        
        // 处理负数
        "neg    eax",
        // "push   '-'",                // 压入负号
        // "inc    ecx",                // 计数器+1
        std::format("jmp    digit_loop_start{}",id[1]),
        
        std::format("positive_label{}:",id[0]),
        // 处理0的特殊情况
        "test   eax, eax",
        std::format("jnz    digit_loop_start{}",id[1]),
        "sub    rsp , 1",
        "mov byte ptr [rsp] , '0'",
        "inc    ecx",
        std::format("jmp    prepare_write{}",id[2]),
        
        std::format("digit_loop_start{}:",id[1]),
        // 将数字各位ASCII码压入栈（反向）
        "mov    edi, 10",
        std::format("digit_loop{}:",id[3]),
        "xor    edx, edx",          // 清零edx用于除法
        "div    edi",               // eax = eax/10, edx = eax%10
        "add    dl, '0'",           // 转换为ASCII
        "sub    rsp , 1",
        "mov byte ptr [rsp] , dl",
        "inc    ecx",               // 位数+1
        "test   eax, eax",
        std::format("jnz    digit_loop{}",id[3]),
        //"jnz    digit_loop",        // 如果eax!=0，继续循环
        std::format("prepare_write{}:",id[2]),
        //"prepare_write:",
        "test   ebx,ebx",
        std::format("jnz    going_write{}",id[4]),
        //"jnz    going_write",
        "sub    rsp , 1",
        "mov byte ptr [rsp] , '-'",
        "inc    ecx",
        std::format("going_write{}:",id[4]),
        // 准备系统调用参数
        "mov    rsi, rsp",          // rsi = 字符串地址（栈顶）
        "mov    edx, ecx",          // edx = 字符数
        "mov    edi, 1",            // edi = 文件描述符（1=STDOUT）
        "mov    eax, 1",            // eax = 系统调用号（1=write）
        "push  rcx",
        "mov    rbx , rsp",         //确保rsp16字节对齐，并且保存现场
        "sub    rsp , 32",
        "and    rsp , -16",
        "syscall",
        "mov    rsp , rbx",
        "pop  rcx",
        "lea    rsp, [rsp + rcx]",
        
        // 恢复所有寄存器
        "pop    rbp",
        "pop    rsi",
        "pop    rdi",
        "pop    rdx",
        "pop    rcx",
        "pop    rbx",
        "pop    rax"
        
    };
    BlockCtx.ASMcode.insert(BlockCtx.ASMcode.end(),assembly.begin(),assembly.end());
    return true;
}

bool callOpASMGen(IR::callOpInst& op, size_t InstIdx ,FuncASMGenerator & ASMGen,BaseBlockContext & BlockCtx ,IR::IRBaseBlock & IRblock) {
    std::vector<std::variant<X64Register, int64_t>> ABIplace = arrangeArgPlace(op.srcs);
    std::unordered_set<X64Register> occupied;
    for(int i = 0 ; i < ABIplace.size() ; i++) {
        if(std::holds_alternative<int64_t>(ABIplace[i])) {
            std::cerr<<"生成失败，暂不支持栈传参\n";
            return false;
        }
        BlockCtx.MoveOperandToTargetReg({std::get<X64Register>(ABIplace[i])},op.srcs[i],InstIdx,occupied);
        occupied.insert(std::get<X64Register>(ABIplace[i]));
    }
    //保存已使用的caller save寄存器
    std::unordered_set<X64Register> caller_save = {
        RAX,RCX,RDX,RSI,RDI,R8,R9,R10,R11,
        XMM0,XMM1,XMM2,XMM3,XMM4,XMM5,XMM6,XMM7,XMM8,XMM9,XMM10,
        XMM11,XMM12,XMM13,XMM14,XMM15};
    for (auto reg : occupied) {
        if(x32Tox64(reg) != NONE)
            caller_save.erase(x32Tox64(reg));
        else 
            caller_save.erase(reg);
    }
    auto checkLastUse = [&](IR::Operand & op){
        return InstIdx >= BlockCtx.IRlocMap[op]->usedIdx.back();
    };

    auto matchRequire = [](const std::unordered_set<X64Register> & target,const std::unordered_set<X64Register> & contrains,X64RegStatus & regStatus) {
        for(const auto & tg : target) {
            if(regStatus.is_reg_name(tg)) {
                bool allowed = true;
                for(const auto & constrain : contrains) {
                    if(regStatus.is_reg_name(constrain)) {
                        allowed = false;
                        break;
                    }
                }
                if(allowed) return tg;
                else return NONE;
            }
        }
        return NONE;
    };
    for (auto& regStatus : BlockCtx.RegFile) {
        auto ret = matchRequire(caller_save, occupied, regStatus);
        if (ret!=NONE && !regStatus.is_Reg_valid(InstIdx) && !checkLastUse(regStatus.occupyOperand)) {
            BlockCtx.spill(regStatus, InstIdx);
        }
    }

    //对齐16字节
    int64_t misalignment = (-BlockCtx.rspOffset) % 16;
    if (misalignment != 0) {
        int64_t adjust = 16 - misalignment;//正数
        BlockCtx.emit(std::format("sub rsp,{}",adjust));
        BlockCtx.rspOffset -= adjust;
    }
    //call



    if(op.targetFUNC.memplace == IR::Operand::GLOBAL) {
        BlockCtx.emit(std::format("call {}",toString_view(op.targetFUNC.LiteralName)));
    } else {
        auto reg1 = BlockCtx.MoveOperandToTargetReg(GPRs64,op.targetFUNC,InstIdx,occupied);
        BlockCtx.emit(std::format("call [{}]",Eformat(reg1)));
    }
    for(int i = 0 ; i < ABIplace.size() ; i++) {
        if(std::holds_alternative<int64_t>(ABIplace[i])) {
            std::cerr<<"生成失败，暂不支持栈传参\n";
            return false;
        }
        BlockCtx.ClearOcuupy(std::get<X64Register>(ABIplace[i]));
    }
    if(op.ret.datatype == IR::Operand::noneinit) {

    } else if(op.ret.datatype == IR::Operand::i32) {
        BlockCtx.DeclareOcuupy(EAX,op.ret);
    } else if(op.ret.datatype == IR::Operand::ptr) {
        BlockCtx.DeclareOcuupy(RAX,op.ret);
    } else if(op.ret.datatype == IR::Operand::f32) {
        BlockCtx.DeclareOcuupy(XMM0,op.ret);
    }
    return true;
    
    
}

bool FuncASMGenerator::BaseBlockASMGen(BaseBlockContext & BlockCtx ,IR::IRBaseBlock & IRblock) {
    //调用前，BlockCtx已完成初始化，可以直接用
    auto & Inst = IRblock.Insts;
    auto & asmCode = BlockCtx.ASMcode;
    for(size_t instIdx = 0 ; instIdx<Inst.size();instIdx++) {
        auto & inst = Inst[instIdx];
        bool result = std::visit(overload {
            [&](IR::BinaryOpInst& op) {
                BinaryOpASMGen(op,instIdx,*this,BlockCtx,IRblock);
                return true;
            },
            [&](IR::callOpInst& op) {
                callOpASMGen(op,instIdx,*this,BlockCtx,IRblock);
                return true;
    
            },
            [&](IR::allocaOpInst& op) {
                //跳过，已经被处理过了
                return true;
            },
            [&](IR::memOpInst& op) {
                //注意操作数类型
                memOpASMGen(op,instIdx,*this,BlockCtx,IRblock);
                return true;
                
            },
            [&](IR::retInst& op) {
                //到达函数末尾  //返回值ABI要求
                if(op.src.datatype == IR::Operand::i32) {
                    BlockCtx.MoveOperandToTargetReg({EAX},op.src,instIdx);
                }
                if(op.src.datatype == IR::Operand::f32) {
                    BlockCtx.MoveOperandToTargetReg({XMM0},op.src,instIdx);
                }
                if(op.src.datatype == IR::Operand::ptr) {
                    BlockCtx.MoveOperandToTargetReg({EAX},op.src,instIdx);
                }
                BlockCtx.RETrestore();
                BlockCtx.emit("ret");
                return true;
            },
            [&](IR::printInst& op) {
                printASMGen(op,instIdx,BlockCtx);
                return true;
                
            },
            [&](IR::BrInst& op) {
                //到达函数末尾了
                //如果是有条件跳转，验证上一条是cmp，并且根据那个类型进行设置J类型
                if(!op.is_conditional) {
                    asmCode.push_back("jmp "+toString(op.trueLabel.format()));
                    //进行栈同步，总是对后继进行同步，如果后继没有生成，则说明后继会继承当前（深度优先生成）
                    //无条件跳转，后继唯一
                    auto tar = *IRblock.successor.begin();
                    if(!this->BlockInfoMap[tar].generated) return true;

                    auto syncASM = BlockCtx.sync_to(*this->BlockInfoMap[tar].initialCtx.get());
                    asmCode.insert(asmCode.end(),syncASM.begin(),syncASM.end());

                } else {
                    //查找true后继 如果需要修改，则jmp到一个临时标签，修改完了，再jmp到之前的标签
                    string jmpType;
                    auto cmpinst = std::get<IR::BinaryOpInst>(Inst[instIdx-1]);
                    switch (cmpinst.cmp_type) {
                    case IR::BinaryOpInst::cmpType::EQ:
                        jmpType = "je";
                        break;
                    case IR::BinaryOpInst::cmpType::G:
                        jmpType = "jg";
                        break;
                    case IR::BinaryOpInst::cmpType::GE:
                        jmpType = "jge";
                        break;
                    case IR::BinaryOpInst::cmpType::L:
                        jmpType = "jl";
                        break;
                    case IR::BinaryOpInst::cmpType::LE:
                        jmpType = "jle";
                        break;
                    case IR::BinaryOpInst::cmpType::NE:
                        jmpType = "jne";
                        break;
                    default:
                        std::cerr<<"ASM不支持的比较\n";
                        return false;
                    }
                    IR::IRBaseBlock * trueBlock;
                    IR::IRBaseBlock * falseBlock;
                    for(auto * successor : IRblock.successor ) {
                        if(successor->enterLabel == op.trueLabel) {
                            trueBlock = successor;
                            break;
                        }
                    }

                    //查找false后继
                    for(auto * successor : IRblock.successor ) {
                        if(successor->enterLabel == op.falseLabel) {
                            falseBlock = successor;
                            break;
                        }
                    }
                    string asmJ;
                    std::vector<string> asmT;
                    std::vector<string> asmF;
                    if(this->BlockInfoMap[trueBlock].generated) {
                        //同步
                        //生成随机label
                        string randomLabel = std::format("jsync__{}",BlockCtx.next_id++);
                        auto syncASM = BlockCtx.sync_to(*this->BlockInfoMap[trueBlock].initialCtx.get());
                        if(!syncASM.empty()) {
                            asmJ = std::format("{} {}",jmpType,randomLabel);
                            asmT = syncASM;
                            asmT.push_back(std::format("jmp {}",toString_view(op.trueLabel.format())));
                        }
                        else {
                            asmJ = std::format("{} {}",jmpType,toString_view(op.trueLabel.format()));
                        }
                        
                    } else {
                        //不同步
                        asmJ = std::format("{} {}",jmpType,toString_view(op.trueLabel.format()));
                    }
                    
                    if(this->BlockInfoMap[falseBlock].generated) {
                        //同步
                        auto syncASM = BlockCtx.sync_to(*this->BlockInfoMap[falseBlock].initialCtx.get());
                        if(!syncASM.empty()) {
                            asmF = syncASM;
                        }
                        else {
                            asmF.emplace_back(std::format("jmp {}",toString_view(op.falseLabel.format())));
                        }
                    } else {
                        //不同步
                        asmF.emplace_back(std::format("jmp {}",toString_view(op.falseLabel.format())));
                    }
                    asmCode.push_back(asmJ);
                    asmCode.insert(asmCode.end(),asmF.begin(),asmF.end());
                    asmCode.insert(asmCode.end(),asmT.begin(),asmT.end());
                }
                return true;
                
            },
            [&](IR::phiInst& op) {
                std::cerr<<"phi asm code not implement\n";
                return false;
                
            },
            [&](IR::labelInst& op) {
                asmCode.push_back(toString(op.label.format())+":");
                return true;            
            }
        }, inst);
        if(!result) return false;
    }
    return true;
}

void test_X64ASM_main() {
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    

    auto astbase = parserGen(
        u8"C:/code/CPP/Compiler-Lab/grammar/grammar.txt",
        u8"C:/code/CPP/Compiler-Lab/grammar/terminal.txt",
        u8"C:/code/CPP/Compiler-Lab/grammar/SLR1ConflictReslove.txt"
    );

    std::ofstream o("output.json");
    nlohmann::json j = astbase;
    o << std::setw(4) << j;  // std::setw(4) 控制缩进
    o.close();
    std::ifstream file("output.json");
    nlohmann::json j2;
    file >> j2;  // 从文件流解析
    ASTbaseContent astbase2 = j2;
    AST::AbstractSyntaxTree astT(astbase2);
    std::string myprogram = R"(
    int q;
    int calculate(int a, int b,int c , int d[][2][3]) {
        int result = q;
        int temp;
        result = a + b + c;
        {
            temp = result + 2;
            result = temp;
        }
        d[3][1][2] = temp;
        return calculate(a,b,c,d);
    }

    void swap(int *a,int *b) {
        int temp = *a;
        *a = *b;
        *b = temp;
        return;
    }
    int main() {
        int y;
        int x;
        int **z;
        int *lr = &y;
        y = 24;
        x = 12;
        swap(&x,&y);
        if(x > y )
        {
            print 1;
        }
        else 
        {
            print 2;
        }
        print x*y;
        print y;
        int a[30][30];
        a[x][y] = 999;
        print a[24][12];
        return x;
    }

    float acc() {
    int a = 1;
    int b = 2;
    a = 10 + b;
    b = 20 + a;

    if (a < b) {
        return 1 ;
    }
    return 0;
}
    )";
    auto ss = Lexer::scan(toU8str(myprogram));
    for(int i= 0 ;i < ss.size() ; i++) {
        auto q = ss[i];
        std::cout<<"["<<toString(q.type)<<" "<<toString(q.value)<<" "<<i<<"]";
    }
    std::cout<<"\n";
    astT.BuildSpecifiedAST(ss);
    //printCommonAST(astT.root);
    AST::mVisitor v;
    //astT.root->accept(v);
    std::cout<<std::endl;
        std::u8string myprogram2 = u8R"(
    int factorial(int n,float b) {
        if (n == 0) {
            return 1;
        }
            return n * factorial(n + -1,0.0);
        }

    int main() {
        int a;
        int * b;
        int c;
        b = &a;
        while ( a < c) {
            a = a + 1;
        }
        if (a<c) {
            a = a + 1 ;
        }
        else {
            c = c + 1;
        }
        return factorial(5,5);
    }

    )";
    // auto ss2 = Lexer::scan(toU8str(myprogram2));
    // for(int i= 0 ;i < ss2.size() ; i++) {
    //     auto q = ss2[i];
    //     std::cout<<"["<<toString(q.type)<<" "<<toString(q.value)<<" "<<i<<"]";
    // }
    // std::cout<<std::endl;
    // astT.BuildSpecifiedAST(ss2);

    Semantic::ASTContentVisitor v2;
    AST::ConstantFoldingVisitor v3;
    Semantic::SymbolTableBuildVisitor v4;
    Semantic::TypingCheckVisitor v5;
    Semantic::SemanticSymbolTable tmp;
    //v2.moveSequence = true;
    astT.root->accept(v3);
    astT.root->accept(v2);
    
    v4.build(&tmp,&astT);
    tmp.checkTyping();
    astT.root->accept(v2);
    Semantic::ExprTypeCheckMap ExprMap;
    v5.build(&tmp,&astT,&ExprMap);
    tmp.arrangeMem();
    tmp.printTable();
    IR::IRContent irbase;
    IR::EntryOperandMap entry_map;
    IR::IRGenVisitor v6;
    v6.build(&tmp,&astT,&irbase,&ExprMap,&entry_map);
    irbase.print();
    std::ofstream asmO("out.s");
    X64ASMGenerator g1;
    g1.genASM(irbase,asmO);

    std::cout<<"stop\n";
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Function took " 
              << duration.count() << " microseconds (" 
              << duration.count() / 1000.0 << " milliseconds) to execute.\n";
    return;
    
}

}