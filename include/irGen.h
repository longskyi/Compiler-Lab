#ifndef LCMP_IR_GEN_HEADER
#define LCMP_IR_GEN_HEADER
#include "AST/AST.h"
#include "Semantic.h"
#include "templateUtil.h"
namespace IR {

using std::u8string;

using OperandId = StrongId<struct OperandIdTag>;

enum Datatype {
        noneinit,
        i32,
        ptr,    //(i64)
        f32,
};

class Operand {
private:
    inline static size_t next_id = 1;
public:
    // id=0说明未初始化（或操作数为空）
    inline Operand() : datatype(noneinit),memplace(STACK),unique_id(OperandId(0)) {}
    inline Operand(const Operand& other ) = default;
    inline bool empty() const {
        return datatype == noneinit || unique_id == 0;
    }
    enum OperandType {
        noneinit,
        i32,
        ptr,    //(ptr)
        f32,
        LABEL,
    };
    enum MemPlace{  //查找方式
        IMM, //立即数
        GLOBAL, //全局
        STACK,    //栈
    };
    OperandType datatype = noneinit;
    MemPlace memplace = IMM;
    OperandId unique_id;
    u8string LiteralName;
    int64_t valueI = 0 ;
    float valueF = 0.0f;
    inline static Operand allocOperand(u8string start_with_ = u8"",OperandType datatype_ = noneinit , MemPlace pos = STACK) {
        auto newOperand = Operand();
        newOperand.datatype = datatype_;
        newOperand.memplace = pos;
        newOperand.unique_id = OperandId(next_id++);
        newOperand.LiteralName = start_with_;

        return newOperand;
    }
    inline u8string format() const {
        if (empty()) {
            return u8"_";
        }

        u8string ret;
        switch (memplace) {
        case MemPlace::GLOBAL:
            // 全局变量格式: 类型@LiteralName（忽略 unique_id）
            switch (datatype) {
            case OperandType::i32:  ret += u8"i32@"; break;
            case OperandType::ptr:  ret += u8"ptr@"; break;
            case OperandType::f32:  ret += u8"f32@"; break;
            case OperandType::LABEL: ret += u8"L@";   break;
            default:                ret += u8"@";    break;
            }
            ret += LiteralName;  // 直接使用 LiteralName（如 @global_var）
            return ret;

        case MemPlace::IMM:
            // 立即数格式: 直接打印值（无 unique_id）
            switch (datatype) {
            case OperandType::i32:  return toU8str(std::to_string(valueI));
            case OperandType::f32:  return toU8str(std::to_string(valueF));
            default:                return u8"_";  // 非数值类型不合法
            }

        case MemPlace::STACK:
            // 栈变量格式: 类型%[Literal_]unique_id
            switch (datatype) {
            case OperandType::noneinit: ret += u8"NULL%";   break;
            case OperandType::i32:      ret += u8"i32%";    break;
            case OperandType::ptr:      ret += u8"ptr%";    break;
            case OperandType::f32:      ret += u8"f32%";    break;
            case OperandType::LABEL:    ret += u8"L%";      break;
            }
            if (LiteralName.empty()) {
                ret += toU8str(std::to_string(unique_id));  // %42
            } else {
                ret += LiteralName + u8"_" + toU8str(std::to_string(unique_id));  // %var_42
            }
            return ret;
        }
        return u8"_";
    }
    bool validate() const {
        // 1. 校验无效类型组合
        if (datatype == OperandType::noneinit && unique_id != 0) {
            return false;  // 未初始化但分配了ID
        }
        // 2. 校验立即数（IMM）的合法性
        if (memplace == MemPlace::IMM) {
            if (datatype == OperandType::ptr || datatype == OperandType::LABEL) {
                return false;  // 指针/LABEL不能是立即数
            }
            if (datatype == OperandType::i32 && (valueI < INT32_MIN || valueI > INT32_MAX)) {
                return false;  // i32立即数越界
            }
        }
        // 3. 校验全局变量/栈变量的名称
        if (memplace == MemPlace::GLOBAL || memplace == MemPlace::STACK) {
            if (LiteralName.empty()) {
                return false;  // 必须有名
            }
        }
        // 4. 校验LABEL类型必须关联名称
        if (datatype == OperandType::LABEL && LiteralName.empty()) {
            return false;
        }
        return true;
    }
};

struct BinaryOpInst {
    enum MainOP {
        add,
        mul,
        COPY,
        cast,
        cmp,
        //暂不支持sub
    };
    enum cmpType {
        EQ,
        GE,
        G,
        LE,
        L
    };
    enum OPtype {
        i32,
        ptr, //(ptr)
        f32,
        // cast类型
        i32Toptr,
        ptrToi32,
        f32Toi32,
        i32Tof32,
        // cast类型 end
    };
    MainOP op;
    OPtype op_type;
    cmpType cmp_type;
    Operand dst;
    Operand src1;
    Operand src2;
    inline bool validate() const {
        // 校验操作数类型匹配
        if (op == MainOP::cmp && cmp_type == cmpType::EQ) {
            if (src1.datatype != src2.datatype) {
                return false;  // 比较运算类型必须相同
            }
        }
        // 校验类型转换合法性
        if (op == MainOP::cast) {
            if ((op_type == OPtype::i32Toptr && src1.datatype != Operand::OperandType::i32) ||
                (op_type == OPtype::ptrToi32 && src1.datatype != Operand::OperandType::ptr)) {
                return false;  // 非法类型转换
            }
        }
        return dst.validate() && src1.validate() && src2.validate();
    }
};

struct callOpInst {
    Operand ret;
    Operand targetFUNC; //ptr
    std::vector<Operand> srcs;

    inline bool validate() const {
        if (targetFUNC.datatype != Operand::OperandType::ptr) {
            return false;  // 目标必须是函数指针
        }
        // 校验返回值类型（如果非void）
        if (!ret.empty() && ret.datatype == Operand::OperandType::noneinit) {
            return false;
        }
        // 校验所有参数
        for (const auto& arg : srcs) {
            if (!arg.validate()) return false;
        }
        return true;
    }
};

struct allocaOpInst {
    Operand ret;
    size_t size;
    size_t offset;
    size_t align;
};

struct memOpInst {
    enum OP {
        noneInit,
        load,
        store,
    };
    enum Datatype {
        noneinit,
        i32,
        ptr,    //(ptr)
        f32,
    };
    OP memOP;
    Datatype datatype;
    Operand src;  // 对于 store，src 是值；对于 load，src 是地址
    Operand dst;  // 对于 store，dst 是地址；对于 load，dst 是结果
    size_t align; // 必须添加对齐
    bool validate() const {
        if (memOP == OP::load) {
            if (dst.datatype != static_cast<Operand::OperandType>(datatype)) {
                return false;  // load结果类型需匹配
            }
        } else if (memOP == OP::store) {
            if (src.datatype != static_cast<Operand::OperandType>(datatype)) {
                return false;  // store值类型需匹配
            }
        }
        return src.validate() && dst.validate();
    }
};

struct retInst {
    Operand src; //可为空
};

struct BrInst {
    bool is_conditional = false;
    Operand condition;
    Operand label1;
    Operand label2;
    bool validate() const {
        if (is_conditional) {
            if (condition.datatype != Operand::OperandType::i32) {
                return false;  // 条件必须是i32
            }
        }
        // 标签必须是LABEL类型
        if (label1.datatype != Operand::OperandType::LABEL || 
            (is_conditional && label2.datatype != Operand::OperandType::LABEL)) {
            return false;
        }
        return true;
    }
};

struct phiInst {
    Operand dst;
    std::vector<std::pair<Operand,Operand>> srcs; // tag - operand    
};

struct labelInst {
    Operand label;
};


using IRinst = std::variant<
    BinaryOpInst,
    callOpInst,
    allocaOpInst,
    memOpInst,
    retInst,
    BrInst,
    phiInst,
    labelInst>;


inline u8string IRformat(IRinst inst) {
    return std::visit(overload {
        // 1. 二元运算指令 (add/mul/cmp/cast)
        [](const BinaryOpInst& op) -> u8string {
            u8string ret;
            switch (op.op) {
            case BinaryOpInst::MainOP::add:
                ret = u8"add ";
                break;
            case BinaryOpInst::MainOP::mul:
                ret = u8"mul ";
                break;
            case BinaryOpInst::MainOP::cmp: {
                u8string cmp_op;
                switch (op.cmp_type) {
                case BinaryOpInst::cmpType::EQ: cmp_op = u8"eq "; break;
                case BinaryOpInst::cmpType::GE: cmp_op = u8"ge "; break;
                case BinaryOpInst::cmpType::G:  cmp_op = u8"gt "; break;
                case BinaryOpInst::cmpType::LE: cmp_op = u8"le "; break;
                case BinaryOpInst::cmpType::L:  cmp_op = u8"lt "; break;
                default: cmp_op = u8""; break;
                }
                ret = u8"cmp." + cmp_op;
                break;
            }
            case BinaryOpInst::MainOP::cast: {
                u8string cast_op;
                switch (op.op_type) {
                case BinaryOpInst::OPtype::i32Toptr: cast_op = u8"i32_to_ptr "; break;
                case BinaryOpInst::OPtype::ptrToi32: cast_op = u8"ptr_to_i32 "; break;
                case BinaryOpInst::OPtype::f32Toi32: cast_op = u8"f32_to_i32 "; break;
                case BinaryOpInst::OPtype::i32Tof32: cast_op = u8"i32_to_f32 "; break;
                default: cast_op = u8""; break;
                }
                ret = u8"cast." + cast_op;
                break;
            }
            case BinaryOpInst::MainOP::COPY:
                ret = u8"copy ";
                break;
            default:
                return u8"";
            }
            ret += op.dst.format() + u8" = " + op.src1.format();
            if (op.op != BinaryOpInst::MainOP::cast) {
                ret += u8", " + op.src2.format();
            }
            return ret;
        },
        // 2. 函数调用指令
        [](const callOpInst& call) -> u8string {
            u8string ret = u8"call ";
            if (!call.ret.empty()) {
                ret += call.ret.format() + u8" = ";
            }
            ret += call.targetFUNC.format() + u8"(";
            for (size_t i = 0; i < call.srcs.size(); ++i) {
                if (i > 0) ret += u8", ";
                ret += call.srcs[i].format();
            }
            ret += u8")";
            return ret;
        },

        // 3. 内存分配指令
        [](const allocaOpInst& alloc) -> u8string {
            return alloc.ret.format() + u8" = alloca " + 
                toU8str(std::to_string(alloc.size)) + u8", align " + 
                toU8str(std::to_string(alloc.align));
        },

        // 4. 内存读写指令
        [](const memOpInst& mem) -> u8string {
            if (mem.memOP == memOpInst::OP::load) {
                return mem.dst.format() + u8" = load " + mem.src.format() + 
                       u8", align " + toU8str(std::to_string(mem.align));
            } else if (mem.memOP == memOpInst::OP::store) {
                return u8"store " + mem.src.format() + u8", " + 
                       mem.dst.format() + u8", align " + toU8str(std::to_string(mem.align));
            }
            return u8"";
        },

        // 5. 返回指令
        [](const retInst& ret) -> u8string {
            if (ret.src.empty()) {
                return u8"ret void";
            }
            return u8"ret " + ret.src.format();
        },

        // 6. 分支指令
        [](const BrInst& br) -> u8string {
            if (!br.is_conditional) {
                return u8"br " + br.label1.format();
            }
            return u8"br " + br.condition.format() + u8", " + 
                   br.label1.format() + u8", " + br.label2.format();
        },

        // 7. Phi节点指令
        [](const phiInst& phi) -> u8string {
            u8string ret = phi.dst.format() + u8" = phi ";
            for (size_t i = 0; i < phi.srcs.size(); ++i) {
                if (i > 0) ret += u8", ";
                ret += u8"[" + phi.srcs[i].second.format() + u8", " + 
                       phi.srcs[i].first.format() + u8"]";
            }
            return ret;
        },

        // 8. 标签指令
        [](const labelInst& label) -> u8string {
            return label.label.format() + u8":";
        },

        // 默认情况
        [](auto&&) -> u8string { return u8""; }
    }, inst);
}

struct GlobalVarIR {
    u8string name;
    size_t size;
    size_t align;
    Operand VarPos;
};

struct FunctionIR {
    u8string funcName;
    Datatype retType;
    std::vector<Datatype> Args;
    std::vector<IRinst> block;
    Operand FuncPos;
};

class IRContent {
    std::vector<GlobalVarIR> globalVar;
    std::vector<FunctionIR> FunctionIR;
    void print() const {

    }

};

//在AST的id Node上绑定了 SymbolEntry ptr，这里继续SymbolEntry -> Operand 映射
using EntryOperandMap = std::unordered_map<Semantic::SymbolEntry *,Operand>;

//生成IR
class IRGenVisitor : public AST::ASTVisitor {
public:
    Semantic::SemanticSymbolTable * rootTable;
    Semantic::SemanticSymbolTable * currentTable;
    IRContent * IRbase;
    Semantic::ExprTypeCheckMap * ExprTypeMap;
    EntryOperandMap * EntrySymMap;

    bool build() {
        return true;
    }
};


}

#endif