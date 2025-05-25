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
    inline bool is_Reg_valid(size_t InstIdx) {
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
    inline bool is_Reg_dirty() {
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
    inline size_t next_use_idx(size_t InstIdx) {
        if(is_Reg_valid(InstIdx)) return 0;
        if(!occupyOperandContext) {
            std::cerr<<"寄存器分配错误，不可用的operand context\n";
        }
        //应该不可能这么多inst
        if(occupyOperandContext->will_escape) return std::numeric_limits<int>::max()/2;
        auto & useVec = occupyOperandContext->usedIdx;
        auto it = std::upper_bound(useVec.begin(),useVec.end(),InstIdx);
        if(it == useVec.end()) {
            return useVec.back();
        } 
        return *it;
    }
    inline bool is_reg_name(X64Register name) {
        if(name == this->Name) return true;
        for(auto q : aliasName) {
            if(q == name) return true;
        }
        return false;
    }
    inline void occupy(X64Register usedName, IR::Operand & operand) {
        this->occupyOperand = operand;
        this->OccupyName = usedName;
    }
    inline void clear() {
        this->occupyOperand.clear();
        this->occupyOperandContext = nullptr;
    }
    inline bool isXMM() {
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


inline std::vector<std::variant<X64Register,int64_t>> arrangeArgPlace(IR::FunctionIR & funcIR) {
    std::vector<std::variant<X64Register,int64_t>> ret;
    auto & args = funcIR.Args;
    //是从左到右的顺序
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
        }
        if(args[i].datatype == IR::Operand::f32) {
            switch (intRegUsed) {
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
        }
        if(args[i].datatype == IR::Operand::ptr) {
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
        } else {
            std::cerr<<"不支持的数据类型传参\n";
        }
    }
    return ret;
}


class BaseBlockContext { //Block上下文
private:
    inline static size_t next_id = 0; //唯一id方便使用
public:
    std::vector<X64RegStatus> RegFile;  //排序有讲究，尽量把通用寄存器放前头
    std::unordered_map<IR::Operand,std::unique_ptr<OperandContext>> IRlocMap;
    int64_t rspOffset = 0; //当前rsp相比ret（rbp默认位置）的offset 一般总是为负 //注意控制块中间的栈帧同步与spill问题
    std::vector<string> dataSection;    //不含section头，只包含定义
    std::vector<string> ASMcode;    //已经是最终输出，不进行格式化
    //target是目标寄存器，contrains是要求不更改这些寄存器，返回实际分配到的寄存器
    void spill(X64RegStatus& src,size_t InstIdx) {
        // 确保栈对齐
        if (src.Basewidth == 128) {  // XMM寄存器需要16字节对齐
            // 检查当前栈是否16字节对齐（rspOffset是否为-16的倍数）
            if (rspOffset % 16 != 0) {
                int64_t adjust = (-rspOffset % 16);  // 需要调整的量
                rspOffset -= adjust;
                ASMcode.push_back("sub rsp, " + std::to_string(adjust));
            }
        }
        // 分配栈空间并生成mov指令
        rspOffset -= src.Basewidth / 8;  // width是位数，转为字节    
        if (src.isXMM()) {
            // XMM寄存器使用movaps/movups
            bool isAligned = (rspOffset % 16) == 0;
            std::string movInstr = isAligned ? "movaps" : "movups";
            ASMcode.push_back(movInstr + " [rsp" + 
                            (rspOffset < 0 ? std::to_string(rspOffset) : "+"+std::to_string(rspOffset)) + 
                            "], " + Eformat(src.Name));
        } else {
            // 通用寄存器使用mov
            ASMcode.push_back("mov [rsp" + 
                            (rspOffset < 0 ? std::to_string(rspOffset) : "+"+std::to_string(rspOffset)) + 
                            "], " + Eformat(src.Name));
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
    X64Register allocReg(const std::unordered_set<X64Register> & target,const std::unordered_set<X64Register> & contrains , size_t InstIdx) {
        //分为三道pass，第一道pass查看有没有空的，有的话则返回
        //第二道pass，如果第一道pass失败，则尝试使用dirty的reg
        //第三道pass，如果还是失败，则使用最后一次使用最晚的，spill出去并返回
        for (auto& regStatus : RegFile) {
            if (target.count(regStatus.Name) && 
                    !contrains.count(regStatus.Name) && 
                    regStatus.is_Reg_valid(InstIdx)) {
                    return regStatus.Name;
            }
        }
        for (auto& regStatus : RegFile) {
            if (target.count(regStatus.Name) && 
                !contrains.count(regStatus.Name) && 
                regStatus.is_Reg_dirty()) {
                //脏寄存器需要spill出去
                spill(regStatus, InstIdx);
                return regStatus.Name;
            }
        }

        X64RegStatus* farthestReg = nullptr;
        size_t farthestUse = 0;
        
        for (auto& regStatus : RegFile) {
            if (target.count(regStatus.Name) && !contrains.count(regStatus.Name)) {
                size_t nextUse = regStatus.next_use_idx(InstIdx);
                if (nextUse > farthestUse) {
                    farthestUse = nextUse;
                    farthestReg = &regStatus;
                }
            }
        }
        if (farthestReg) {
            spill(*farthestReg, InstIdx);
            return farthestReg->Name;
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
        X64Register srcReg;
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
                dataSection.push_back(label + ": dd " + std::to_string(src.valueF));
                ASMcode.push_back("movss " + Eformat(targetReg) + ", [" + label + "]");
            }
            else if (src.datatype == IR::Operand::OperandType::ptr) {
                // 64位指针立即数
                ASMcode.push_back("mov " + Eformat(targetReg) + ", " + std::to_string(src.valueI));
            }
        }

        // 处理栈指针偏移（alloca 情况）
        else if (loc.isStackPtr) {
            int64_t offRbp = loc.spillOffset;   // rbp + offRbp
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
        } else if(src.memplace == IR::Operand::GLOBAL) {
            ASMcode.push_back("mov " + Eformat(targetReg) +", " + toString(src.LiteralName));
        }
        // 更新寄存器状态
        for (auto& reg : RegFile) {
            if (reg.is_reg_name(targetReg)) {
                reg.occupy(targetReg, src);
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
            }
        } 
        if(maxSize > 0 ) {
            //分配栈空间
            ASMcode.push_back("sub rsp, " + std::to_string(maxSize));
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
                        regf.occupy(regName,funcIR.Args[i]);
                    }
                }
            }
        }
        auto markDef = [&](IR::Operand & op,size_t idx){
            this->IRlocMap[op]->startIdx = idx;
        };
        auto markUse = [&](IR::Operand & op,size_t idx){
            if(op.memplace == IR::Operand::GLOBAL) {
                this->IRlocMap[op]->startIdx = 0;
            }
            this->IRlocMap[op]->usedIdx.push_back(idx);
        };
        //给所有operand设置 起始和使用 
        for(size_t instIdx = 0 ; instIdx < funcIR.Instblock.size() ; instIdx++) {
            auto & inst = funcIR.Instblock[instIdx];
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
        ASMcode.push_back("add "+ Eformat(RSP)+", "+std::to_string(-this->rspOffset));
    }
};


//暂时不考虑 phi指令问题
class FuncASMGenerator
{
public:
    std::unordered_map<IR::IRBaseBlock *,std::unique_ptr<BaseBlockContext>> BlockCtxMap;
};


inline bool BinaryOpASMGen(IR::BinaryOpInst& op, size_t InstIdx ,FuncASMGenerator & ASMGen,BaseBlockContext & BlockCtx ,IR::IRBaseBlock & IRblock) {
    if(op.op == IR::BinaryOpInst::add) {
        if(op.op_type == IR::BinaryOpInst::i32) {

        }
        if(op.op_type == IR::BinaryOpInst::f32) {
            
        }
        if(op.op_type == IR::BinaryOpInst::ptr) {
            
        }
    } else if(op.op == IR::BinaryOpInst::mul) {
        if(op.op_type == IR::BinaryOpInst::i32) {

        }
        if(op.op_type == IR::BinaryOpInst::f32) {
            
        }
        if(op.op_type == IR::BinaryOpInst::ptr) {
            
        }
    } else if(op.op == IR::BinaryOpInst::cast) {
        if(op.op_type == IR::BinaryOpInst::i32Tof32) {

        }
        if(op.op_type == IR::BinaryOpInst::f32Toi32) {
            
        }
        if(op.op_type == IR::BinaryOpInst::i32Toptr) {
            
        }
        if(op.op_type == IR::BinaryOpInst::ptrToi32) {
            
        }
    } else if(op.op == IR::BinaryOpInst::cmp) {
        //下一条一定是br
        if(op.cmp_type == IR::BinaryOpInst::EQ) {

        }
        if(op.cmp_type == IR::BinaryOpInst::G) {

        }
        if(op.cmp_type == IR::BinaryOpInst::GE) {

        }
        if(op.cmp_type == IR::BinaryOpInst::L) {

        }
        if(op.cmp_type == IR::BinaryOpInst::LE) {

        }
    } else if(op.op == IR::BinaryOpInst::COPY) {
        if(op.op_type == IR::BinaryOpInst::i32Tof32) {

        }
        if(op.op_type == IR::BinaryOpInst::f32Toi32) {
            
        }
        if(op.op_type == IR::BinaryOpInst::i32Toptr) {
            
        }
        if(op.op_type == IR::BinaryOpInst::ptrToi32) {
            
        }
    }
    else {
        std::cerr<<"不支持的binary OP\n";
        return false;
    }
    return true;
}


inline bool BaseBlockASMGen(FuncASMGenerator & ASMGen,BaseBlockContext & BlockCtx ,IR::IRBaseBlock & IRblock) {
    //调用前，BlockCtx已完成初始化，可以直接用
    auto & Inst = IRblock.Insts;
    auto & asmCode = BlockCtx.ASMcode;
    for(size_t instIdx = 0 ; instIdx<Inst.size();instIdx++) {
        auto & inst = Inst[instIdx];
        std::visit(overload {
            [&](IR::BinaryOpInst& op) {
                return true;
            },
            [&](IR::callOpInst& op) {
                return true;
    
            },
            [&](IR::allocaOpInst& op) {
                //跳过，已经被处理过了
                return true;
            },
            [&](IR::memOpInst& op) {
                //注意操作数类型
                if(op.memOP == IR::memOpInst::load) {

                } else if(op.memOP == IR::memOpInst::store) {

                }
                return true;
                
            },
            [&](IR::retInst& op) {
                //到达函数末尾
                return true;
                
            },
            [&](IR::printInst& op) {
                
                return true;
                
            },
            [&](IR::BrInst& op) {
                //到达函数末尾了
                //如果是有条件跳转，验证上一条是cmp，并且根据那个类型进行设置J类型
                if(!op.is_conditional) {
                    asmCode.push_back("jmp "+toString(op.trueLabel.format()));
                    //进行栈同步，总是对后继进行同步，如果后继没有生成，则说明后继会继承当前（深度优先生成）
                } else {

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
    }
    return true;
}

} // namespace ASM



#endif