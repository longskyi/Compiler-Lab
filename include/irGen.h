#ifndef LCMP_IR_GEN_HEADER
#define LCMP_IR_GEN_HEADER
#include "AST/AST.h"
#include "Semantic.h"
#include "templateUtil.h"
#include <unordered_map>
namespace IR {

using std::u8string;

using OperandId = StrongId<struct OperandIdTag>;

enum Datatype {
        noneinit,
        i32,
        ptr,    //(i64)
        f32,
};

inline Datatype TypingSystem2IRType(AST::SymType & Type) {
    switch (Type.basicType) {
        case AST::baseType::INT:
        return Datatype::i32;
        case AST::baseType::ARRAY:
        return Datatype::ptr;
        case AST::baseType::BASE_PTR:
        return Datatype::ptr;
        case AST::baseType::FUNC_PTR:
        return Datatype::ptr;
        case AST::baseType::FUNC:
        return Datatype::ptr;
        case AST::baseType::FLOAT:
        return Datatype::f32;
        case AST::baseType::VOID:
        std::cerr<<"警告：尝试获取VOID的类型\n";
        return Datatype::noneinit;
        case AST::baseType::NonInit:
        std::cerr<<"警告：尝试获取noneinit的类型\n";
        return Datatype::noneinit;
    }
    return noneinit;
}

class Operand {
private:
    inline static size_t next_id = 1;
public:
    // id=0说明未初始化（或操作数为空的情况）
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

    //operand通过unique id唯一跟踪
    bool operator==(const Operand& other) const {
        return unique_id == other.unique_id;
    }

    bool operator!=(const Operand& other) const {
        return !(*this == other);
    }
    inline static Operand allocOperand(OperandType datatype_ = noneinit ,u8string start_with_ = u8"", MemPlace pos = STACK) {
        auto newOperand = Operand();
        newOperand.datatype = datatype_;
        newOperand.memplace = pos;
        newOperand.unique_id = OperandId(next_id++);
        newOperand.LiteralName = start_with_;
        return newOperand;
    }
    inline static Operand allocOperand(Datatype datatype_ = Datatype::noneinit ,u8string start_with_ = u8"", MemPlace pos = STACK);
    inline static Operand allocIMM(OperandType datatype_ = noneinit ,double valuesF = 0.0, int64_t valuesI = 0) {
        auto newOperand = Operand();
        newOperand.datatype = datatype_;
        newOperand.memplace = MemPlace::IMM;
        newOperand.unique_id = OperandId(next_id++);
        newOperand.valueI = valuesI;
        newOperand.valueF = valuesF;
        return newOperand;
    }
    inline static Operand allocLabel(u8string start_with_, MemPlace pos = STACK) {
        auto newOperand = Operand();
        newOperand.datatype = LABEL;
        newOperand.memplace = pos;
        newOperand.unique_id = OperandId(next_id++);
        newOperand.LiteralName = start_with_;
        return newOperand;
    }
    /**
        用于 ptr%a_0 = alloca size 4 offset 4 使用
    */
    inline static Operand allocVAR(u8string start_with_ = u8"",MemPlace pos = STACK) {
        auto newOperand = Operand();
        newOperand.datatype = ptr;
        newOperand.memplace = pos;
        newOperand.LiteralName = start_with_;
        newOperand.unique_id = OperandId(next_id++);
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
        // 3. 校验全局变量的名称
        if (memplace == MemPlace::GLOBAL) {
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

inline Operand::OperandType IRtype2OperandType(Datatype datatype) {
    switch(datatype) {
    case IR::Datatype::f32:
        return Operand::OperandType::f32;
    case IR::Datatype::i32:
        return Operand::OperandType::i32;
    case IR::Datatype::ptr:
        return Operand::OperandType::ptr;
    default:
        std::cerr<<"错误的类型转换12\n";
        return Operand::OperandType::noneinit;
    }
}

inline Operand Operand::allocOperand(Datatype datatype_ ,u8string start_with_, MemPlace pos) {
        auto newOperand = Operand();
        newOperand.datatype = IRtype2OperandType(datatype_);
        newOperand.memplace = pos;
        newOperand.unique_id = OperandId(next_id++);
        newOperand.LiteralName = start_with_;
        return newOperand;
}

}   // namespace IR

namespace std {
    template<>
    struct hash<IR::Operand> {
        size_t operator()(const IR::Operand& op) const noexcept {
            return hash<decltype(op.unique_id)>()(op.unique_id);
        }
    };
}

namespace IR {

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
        EQ,NE,
        GE,G,
        LE,L
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

struct printInst {
    Operand src; //恒为int，后期内联汇编
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
    printInst,
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
        // new 9. print指令
        [](const printInst& ret) -> u8string {
            return u8"print " + ret.src.format();
        },
        // 默认情况
        [](auto&&) -> u8string { return u8""; }
    }, inst);
}

struct GlobalVarIR {
    u8string name;
    size_t size;
    size_t align;
    uint64_t initValue;

    Operand VarPos; //ptr

    u8string format() const {
        u8string ret;
        ret = VarPos.format();
        ret += u8" = global ";
        ret += name;
        ret += toU8str(std::format(" size {}  align {} initValue {}",size,align,initValue));
        return ret;
    }
};

struct FunctionIR {
    u8string funcName;
    Datatype retType;
    std::vector<Datatype> ArgsType;
    std::vector<IRinst> Instblock;
    Operand FuncPos;    //ptr
    // u8string format() const {
    //     u8string ret;
    //     return ret;
    // }
};

class IRContent {
public:
    std::vector<GlobalVarIR> globalVar;
    std::vector<FunctionIR> FunctionIR;
    void print() const {
        for(const auto & gb : globalVar) {
            std::cout<<toString_view(gb.format())<<std::endl;
        }
    }

};

//在AST的id Node上绑定了 SymbolEntry ptr，这里继续SymbolEntry -> Operand 映射
using EntryOperandMap = std::unordered_map<Semantic::SymbolEntry *,Operand>;

//生成IR时候需要的上下文
struct IRNodeContext {
    Operand expr_addr;
    std::vector<Operand> BoolTrueLabel;
    std::vector<Operand> BoolFalseLabel;
    std::deque<IRinst> code;
};

using IRContextMap = std::unordered_map<AST::ASTNode *,IRNodeContext>;


// Declaration
class IRGenVisitor : public AST::ASTVisitor {
public:
    Semantic::SemanticSymbolTable* rootTable;
    Semantic::SemanticSymbolTable* currentTable;
    IRContent* IRbase;
    Semantic::ExprTypeCheckMap* ExprTypeMap;
    EntryOperandMap* EntrySymMap;
    IRContextMap ContextMap;
    std::stack<AST::ASTNode*> ASTStack;

    enum GenStateENUM {
        None,
        globalIdGen,
        FunctionGen,
    };

    GenStateENUM curr_state = None;
    Semantic::SymbolEntry* curr_func_entry;
    FunctionIR* curr_fucntion_ir;
    bool gotError = false;

    // Method declarations
    inline bool build(Semantic::SemanticSymbolTable* symtab, AST::AbstractSyntaxTree* astT, IRContent* irbase, EntryOperandMap* entrymap);
    inline void enter(AST::ASTNode* node) override;
    inline void visit(AST::ASTNode* node) override;
    inline void quit(AST::ASTNode* node) override;
};


//获取idNode对应的Operand
inline Operand fetchIdAddr(AST::SymIdNode * id_ptr , IRGenVisitor * IRGenerator) {
    auto SePtr = static_cast<Semantic::SymbolEntry *>(id_ptr->symEntryPtr);
    auto ret = IRGenerator->EntrySymMap->at(SePtr);
    return ret;
}
//idNode对应的数据类型
inline Datatype fetchIdType(AST::SymIdNode * id_ptr , IRGenVisitor * IRGenerator) {
    auto SePtr = static_cast<Semantic::SymbolEntry *>(id_ptr->symEntryPtr);
    return TypingSystem2IRType(SePtr->Type);
}
//生成cast指令，添加在insts之后，返回cast后的Operand，src不应该是立即数
inline Operand genCastInst(std::deque<IRinst> & insts ,AST::CAST_OP cast_op , Operand src) {
    BinaryOpInst cast_inst;
    cast_inst.op = BinaryOpInst::cast;
    switch(cast_op) {
    case AST::CAST_OP::INT_TO_FLOAT:
    {
        cast_inst.op_type = BinaryOpInst::i32Tof32;
        cast_inst.src1 = src;
        auto dst = Operand::allocOperand(Operand::OperandType::f32);
        cast_inst.dst = dst;
        cast_inst.validate();
        insts.push_back(cast_inst);
        return dst;
    }
    case AST::CAST_OP::INT_TO_PTR:
    {
        cast_inst.op_type = BinaryOpInst::i32Toptr;
        cast_inst.src1 = src;
        auto dst = Operand::allocOperand(Operand::OperandType::ptr);
        cast_inst.dst = dst;
        cast_inst.validate();
        insts.push_back(cast_inst);
        return dst;
    }
    case AST::CAST_OP::PTR_TO_INT:
    {
        cast_inst.op_type = BinaryOpInst::ptrToi32;
        cast_inst.src1 = src;
        auto dst = Operand::allocOperand(Operand::OperandType::i32);
        cast_inst.dst = dst;
        cast_inst.validate();
        insts.push_back(cast_inst);
        return dst;
    }
    case AST::CAST_OP::FLOAT_TO_INT:
    {
        cast_inst.op_type = BinaryOpInst::f32Toi32;
        cast_inst.src1 = src;
        auto dst = Operand::allocOperand(Operand::OperandType::i32);
        cast_inst.dst = dst;
        cast_inst.validate();
        insts.push_back(cast_inst);
        return dst;
    }
    case AST::CAST_OP::FUNC_TO_PTR:
    case AST::CAST_OP::ARRAY_TO_PTR:
    case AST::CAST_OP::PTR_TO_PTR:
    case AST::CAST_OP::NO_OP:
        return src;
    default:
        std::cerr<<"未实现的类型转换\n";
        return src;
    }
}
//生成load指令
inline void genLoadInst(std::deque<IRinst> & insts ,Operand srcAddr,Operand dst) {
    memOpInst load_inst;
    load_inst.memOP = memOpInst::load;
    load_inst.src = srcAddr;
    load_inst.dst = dst;
    switch(dst.datatype)
    {
    case Operand::OperandType::f32 :
        load_inst.datatype = f32;
        break;
    case Operand::OperandType::i32 :
        load_inst.datatype = i32;
        break;
    case Operand::OperandType::ptr :
        load_inst.datatype = ptr;
        break;
    case Operand::OperandType::LABEL :
    case Operand::OperandType::noneinit :
    default:
        std::cerr<<"错误的load指令dst类型\n";
        return;
    }
    insts.push_back(load_inst);
}

//生成store指令
inline void genStoreInst(std::deque<IRinst> & insts ,Operand src,Operand dstAddr) {
    memOpInst store_inst;
    store_inst.memOP = memOpInst::store;
    store_inst.src = src;
    store_inst.dst = dstAddr;
    switch(src.datatype)
    {
    case Operand::OperandType::f32 :
        store_inst.datatype = f32;
        break;
    case Operand::OperandType::i32 :
        store_inst.datatype = i32;
        break;
    case Operand::OperandType::ptr :
        store_inst.datatype = ptr;
        break;
    case Operand::OperandType::LABEL :
    case Operand::OperandType::noneinit :
    default:
        std::cerr<<"错误的load指令dst类型\n";
        return;
    }
    insts.push_back(store_inst);
}

//生成call指令，返回dst地址（对于void，dst为空Operand）
inline Operand genCallInst(std::deque<IRinst> & insts ,AST::SymIdNode * func_id,IRGenVisitor * irgen,std::vector<Operand> srcAddrVec) {
    callOpInst call_inst;
    auto func = fetchIdAddr(func_id,irgen);
    auto typing = static_cast<Semantic::SymbolEntry *>(func_id->symEntryPtr)->Type;
    call_inst.srcs = srcAddrVec;
    call_inst.targetFUNC = func; 
    Operand dst;
    if(typing.eType->basicType != AST::baseType::VOID) {
        dst = Operand::allocOperand(fetchIdType(func_id,irgen),u8"ret");
    }
    call_inst.ret = dst;
    insts.push_back(call_inst);
    return dst;
}
inline Operand::OperandType TypingSystem2OperandType(AST::SymType & Type) {
    return IRtype2OperandType(TypingSystem2IRType(Type));
}

inline bool parseIdInitIRGen (AST::IdDeclare * idDecl_ptr,IRGenVisitor * IRGenerator) {
    auto & Context = IRGenerator->ContextMap;
    std::deque<IRinst> curr_code;
    if(idDecl_ptr->initExpr.has_value()){
        auto idAddr = fetchIdAddr(idDecl_ptr->id_ptr.get(),IRGenerator);
        auto ir = Context.at(idDecl_ptr->initExpr.value().get());
        curr_code.insert(curr_code.end(),std::move_iterator(ir.code.begin()),std::move_iterator(ir.code.end()));
        genStoreInst(curr_code,ir.expr_addr,idAddr);
        Context[idDecl_ptr].code = std::move(curr_code);
        Context.erase(idDecl_ptr->initExpr.value().get());
    }
    return true;
}

inline bool parseStmtIRGen(AST::Stmt * Stmt_ptr,IRGenVisitor * IRGenerator) {
    auto & Context = IRGenerator->ContextMap;
    if(Stmt_ptr->subType == AST::ASTSubType::Assign) {
        std::deque<IRinst> curr_code;
        auto assign_ptr = static_cast<AST::Assign *>(Stmt_ptr);
        //分为两种 id = Expr 和 Expr = Expr
        if(assign_ptr->id_ptr) {
            //id
            auto idAddr = fetchIdAddr(assign_ptr->id_ptr.get(),IRGenerator);
            auto ir = Context.at(assign_ptr->Rexpr_ptr.get());
            curr_code.insert(curr_code.end(),std::move_iterator(ir.code.begin()),std::move_iterator(ir.code.end()));
            genStoreInst(curr_code,ir.expr_addr,idAddr);
            Context[Stmt_ptr].code = std::move(curr_code);
            Context.erase(assign_ptr->Rexpr_ptr.get());
        }
        else {
            //Expr = Expr
            //not implement
        }
    }
    // Branch,
    // Loop,
    // FunctionCall, 下面有expr版本的
    // Return,
    // StmtPrint,
    return true;
}


inline bool parseExprIRGen(AST::Expr * Expr_ptr,IRGenVisitor * IRGenerator) {
    auto & Context = IRGenerator->ContextMap;
    if(Expr_ptr->subType == AST::ASTSubType::ConstExpr) {
        //ConstExpr自动折叠成立即数，立即数读取（float问题）交给后端处理
        auto constExpr_ptr = static_cast<AST::ConstExpr *>(Expr_ptr);
        auto IMMoperand = std::visit( overload {
            [](int v){
                return Operand::allocIMM(Operand::i32,v,v);
            },
            [](float v){
                return Operand::allocIMM(Operand::f32,v,v);
            },
            [](uint64_t v){
                return Operand::allocIMM(Operand::ptr,v,v);
            },}
            ,constExpr_ptr->value);
        //额外处理类型转换
        auto TypingNode = (*IRGenerator->ExprTypeMap)[Expr_ptr];
        auto cast_op = TypingNode.cast_op;
        switch(cast_op) {
        case AST::CAST_OP::FLOAT_TO_INT:
            IMMoperand.datatype = Operand::i32;
            break;
        case AST::CAST_OP::INT_TO_FLOAT:
            IMMoperand.datatype = Operand::f32;
            break;
        case AST::CAST_OP::INT_TO_PTR:
            IMMoperand.datatype = Operand::ptr;
            break;
        case AST::CAST_OP::PTR_TO_INT:
            IMMoperand.datatype = Operand::i32;
            break;
        case AST::CAST_OP::FUNC_TO_PTR:
        case AST::CAST_OP::ARRAY_TO_PTR:
        case AST::CAST_OP::PTR_TO_PTR:
        case AST::CAST_OP::NO_OP:
            break;
        default:
            std::cerr<<"未实现的类型转换\n";
            break;
        }
        IMMoperand.validate();
        Context[Expr_ptr].expr_addr = IMMoperand;   
    }
    if(Expr_ptr->subType == AST::ASTSubType::IdValueExpr) {
        //ConstExpr自动折叠成立即数，立即数读取（float问题）交给更后端处理
        auto idExpr_ptr = static_cast<AST::IdValueExpr *>(Expr_ptr);
        Semantic::TypingCheckNode TypingNode = (*IRGenerator->ExprTypeMap)[Expr_ptr];
        auto cast_op = TypingNode.cast_op;
        std::deque<IRinst> curr_code;
        Operand addrOperand;


        if(idExpr_ptr->behave == AST::ref) {
            //取地址，一定是右值，送变量地址 + 类型转换 PTR_TO_INT
            auto idAddr = fetchIdAddr(idExpr_ptr->id_ptr.get(),IRGenerator);
            //safe check
            switch(cast_op) {
            case AST::CAST_OP::PTR_TO_INT :
                addrOperand = genCastInst(curr_code,cast_op,idAddr);
            case AST::CAST_OP::NO_OP :
                addrOperand = idAddr;
                break;
            default:
                std::cerr<<"不正常的类型转换";
                break;
            }
        }
        else if(idExpr_ptr->behave == AST::direct) {
            //取id，考虑左右值
            if(TypingNode.ret_is_left_value) {
                //左值，送变量地址 + 忽略类型转换 不生成code
                auto idAddr = fetchIdAddr(idExpr_ptr->id_ptr.get(),IRGenerator);
                addrOperand = idAddr;
            } else {
                //右值，load取值 + 类型转换
                auto idAddr = fetchIdAddr(idExpr_ptr->id_ptr.get(),IRGenerator);
                Operand ValOperand = Operand::allocOperand(fetchIdType(idExpr_ptr->id_ptr.get(),IRGenerator),
                                                            idExpr_ptr->id_ptr->Literal);
                genLoadInst(curr_code,idAddr,ValOperand);
                addrOperand = genCastInst(curr_code,cast_op,ValOperand);
            }
        } else {
            std::cerr<<"IdExpr IRGen Not Implement\n";
            return false;
        }

        addrOperand.validate();
        Context[Expr_ptr].expr_addr = addrOperand;   
        Context[Expr_ptr].code = std::move(curr_code);
    }
    if(Expr_ptr->subType == AST::ASTSubType::ArithExpr) {

    }
    if(Expr_ptr->subType == AST::ASTSubType::CallExpr) {
        auto callExpr_ptr = static_cast<AST::CallExpr *>(Expr_ptr);
        auto id_ptr = callExpr_ptr->id_ptr.get();
        const auto & pList = callExpr_ptr->ParamList_ptr->paramList;
        std::vector<AST::Expr *> paramExprList;    //目前Param只支持Expr一种
        std::deque<IRinst> curr_code;
        for(const auto & p : pList) {
            if(p->expr_ptr==nullptr) std::cerr<<"不支持的param类型\n";
            paramExprList.push_back(p->expr_ptr.get());
        }
        std::vector<Operand> paramOperands;
        for(const auto expr_ptr : paramExprList) {
            auto ir = IRGenerator->ContextMap.at(expr_ptr);
            curr_code.insert(curr_code.end(),std::move_iterator(ir.code.begin()),std::move_iterator(ir.code.end()));
            paramOperands.push_back(ir.expr_addr);
            Context.erase(expr_ptr);
        }
        auto ret = genCallInst(curr_code,id_ptr,IRGenerator,paramOperands);
        Context[callExpr_ptr].expr_addr = ret;
        Context[Expr_ptr].code = std::move(curr_code);
    }
    if(Expr_ptr->subType == AST::ASTSubType::DerefExpr) {

    }
    return true;
}

inline bool parseBoolIRGen(AST::ASTBool * Bool_ptr,IRGenVisitor * IRGenerator) {
    auto & Context = IRGenerator->ContextMap;

    return true;
}

//Block , BlockItemList 
inline bool parseCopyNode(AST::ASTNode * node_ptr,IRGenVisitor * IRGenerator) {
    auto & Context = IRGenerator->ContextMap;

    if(node_ptr->subType == AST::ASTSubType::Block) {
        auto * block_ptr = static_cast<AST::Block *>(node_ptr);
        Context[node_ptr].code = std::move(Context[block_ptr->ItemList_ptr.get()].code);
        Context.erase(block_ptr->ItemList_ptr.get());
    }
    if(node_ptr->subType == AST::ASTSubType::BlockStmt) {
        auto * blockS_ptr = static_cast<AST::BlockStmt *>(node_ptr);
        Context[node_ptr].code = std::move(Context[blockS_ptr->block_ptr->ItemList_ptr.get()].code);
        Context.erase(blockS_ptr->block_ptr->ItemList_ptr.get());
    }
    if(node_ptr->subType == AST::ASTSubType::BlockItemList) {
        auto * block_ItemLists_ptr = static_cast<AST::BlockItemList *>(node_ptr);
        std::deque<IRinst> code;

        //BlockItemList.code = sub1.code || sub2.code || sub3.code ……

        for(auto & subItem : block_ItemLists_ptr->ItemList) {
            auto subItemPtr = subItem.get();
            if(Context.count(subItemPtr)) {
                auto & subCode = Context[subItemPtr].code;
                code.insert(code.end(),std::move_iterator(subCode.begin()),std::move_iterator(subCode.end()));
                Context.erase(subItemPtr);
            }   //没有Init Expr的Decl不会有code
        }

    }
    return true;
}


inline allocaOpInst parseAlloca(Semantic::SymbolEntry * symEntry, IRGenVisitor * IRGenerator) {
    allocaOpInst ret;
    if(symEntry->is_block) {
        std::cerr<<"内部错误，尝试对block生成alloca\n";
        return ret;
    }
    ret.size =  symEntry->entrysize;
    ret.align = symEntry->alignment;
    ret.offset = symEntry->offset;
    ret.ret = Operand::allocVAR(symEntry->symbolName);
    (*IRGenerator->EntrySymMap)[symEntry] = ret.ret;
    return ret;
}


//func_or_block_entry
inline void parseFuncAlloca(FunctionIR & funcir,Semantic::SymbolEntry * func_Entry , IRGenVisitor * IRGenerator) {
    auto funcTable = func_Entry->subTable.get();
    auto & args = funcTable->Args;
    auto & entries = funcTable->entries;
    for(auto & arg : args) {
        auto allocaInst = parseAlloca(arg.get(),IRGenerator);
        funcir.Instblock.push_back(allocaInst);
    }
    for(auto & ent : entries) {
        if(ent->is_block) {
            parseFuncAlloca(funcir,ent.get(),IRGenerator);
        } else {
            auto allocaInst = parseAlloca(ent.get(),IRGenerator);
            funcir.Instblock.push_back(allocaInst);
        }
    }
}









// Definitions (all inline)
inline bool IRGenVisitor::build(Semantic::SemanticSymbolTable* symtab, AST::AbstractSyntaxTree* astT, IRContent* irbase, EntryOperandMap* entrymap) {
    rootTable = symtab;
    currentTable = symtab;
    EntrySymMap = entrymap;
    IRbase = irbase;
    //分为两道pass，第一道pass生成所有全局变量
    if(astT->root->Ntype != AST::ASTType::Program) {
        std::cerr<<"内部错误，错误的ast结构\n";
        return false;
    }
    auto globalBlockItemList_ptr = static_cast<AST::Program*>(astT->root.get())->ItemList.get();
    const auto& declList = globalBlockItemList_ptr->ItemList;
    for(const auto& decl : declList) {
        if(decl->subType == AST::ASTSubType::IdDeclare) {
            curr_state = globalIdGen;
            //正向遍历
            GlobalVarIR newvar;
            auto IdNode = static_cast<AST::IdDeclare*>(decl.get());
            auto SymEntry_ptr = static_cast<Semantic::SymbolEntry*>(IdNode->id_ptr->symEntryPtr);
            newvar.align = SymEntry_ptr->alignment;
            newvar.initValue = 0ll;

            if(IdNode->initExpr.has_value()) {
                auto InitExpr = static_cast<AST::ConstExpr*>(IdNode->initExpr->get());
                //std::variant<int, float,uint64_t>
                auto value = InitExpr->value;

                //newvar.initValue = 注意浮点的处理
                std::visit([&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, int>) {
                        // 对于int类型，直接转换为uint64_t，高32位自然为0
                        newvar.initValue = static_cast<uint64_t>(arg);
                    } else if constexpr (std::is_same_v<T, float>) {
                        // 对于float类型，先转换为uint32_t再存入低32位
                        uint32_t floatAsInt;
                        std::memcpy(&floatAsInt, &arg, sizeof(float));
                        newvar.initValue = static_cast<uint64_t>(floatAsInt);
                    } else if constexpr (std::is_same_v<T, uint64_t>) {
                        // 对于uint64_t类型，直接存储
                        newvar.initValue = arg;
                    }
                }, value);
            }
            
            newvar.name = SymEntry_ptr->symbolName;
            newvar.size = SymEntry_ptr->entrysize;
            newvar.VarPos = Operand::allocVAR(newvar.name,Operand::GLOBAL);
            if(EntrySymMap->count(SymEntry_ptr)) {
                std::cerr<<"全局变量分配内部错误\n";
            }
            (*EntrySymMap)[SymEntry_ptr] = newvar.VarPos;
            IRbase->globalVar.push_back(newvar);
        }
        else if(decl->subType == AST::ASTSubType::FuncDeclare) {
            curr_state = FunctionGen;
            auto func = static_cast<AST::FuncDeclare*>(decl.get());
            auto funcBlock = static_cast<AST::FuncDeclare*>(decl.get())->Block_ptr.get();
            curr_func_entry = rootTable->lookup(func->id_ptr->Literal);
            if(!curr_func_entry) {
                std::cerr<<"内部错误，函数定义阶段出现错误\n";
            }
            FunctionIR funcIR;
            funcIR.funcName = func->id_ptr->Literal;
            funcIR.retType = TypingSystem2IRType(*func->funcType.eType.get());
            for(auto& argtype : func->funcType.TypeList) {
                if(argtype->basicType == AST::baseType::VOID) break;
                funcIR.ArgsType.push_back(TypingSystem2IRType(*argtype.get()));
            }
            //funcIR.Instblock;
            funcIR.FuncPos = Operand::allocVAR(funcIR.funcName,Operand::GLOBAL);
            auto SymEntry_ptr = static_cast<Semantic::SymbolEntry*>(func->id_ptr->symEntryPtr);
            if(EntrySymMap->count(SymEntry_ptr)) {
                std::cerr<<"函数分配内部错误\n";
            }
            (*EntrySymMap)[SymEntry_ptr] = funcIR.FuncPos;
            curr_fucntion_ir = &funcIR;
            parseFuncAlloca(funcIR,SymEntry_ptr,this);
            funcBlock->accept(*this);
            IRbase->FunctionIR.emplace_back(std::move(funcIR));
            //树上遍历
        }
    }
    return true;
}

inline void IRGenVisitor::enter(AST::ASTNode* node) {
    if(gotError) return;
    ASTStack.push(node);
}

inline void IRGenVisitor::visit(AST::ASTNode* node) {
    if(gotError) return;
}

inline void IRGenVisitor::quit(AST::ASTNode* node) {
    if(gotError) return;
    ASTStack.pop();
    if(!ASTStack.empty()) {
        auto parent = ASTStack.top();
    }
}






inline void test_IR_main() {
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
    int acc() {
    int a = 1;
    int b = 2;
    a = 10;
    b = 20;
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
    astT.BuildCommonAST(ss);
    //printCommonAST(astT.root);
    AST::mVisitor v;
    //astT.root->accept(v);
    std::cout<<std::endl;

    std::u8string myprogram2 = u8R"(
    int acc =1;
    float **** b;
    int main2(int c [][30] ;float b ) {
        int a = 5;
    }
    int main(int p;float L) {
        int a = 200 * 4.11111;
        int * refa = &a;
        *refa = 4.2;
        a= 5;
        int q = 4;
        if(4 <= 3) {
            int b = 8;
            b = 12 + 20;
        }
        {
            int q = 20 + 14;
            q = 20.2;
            main(4,4.2);
        }
        q = 4.2;
        a = 16 * 5 + 8.9;
        int b = 8;
        int c [10][30];
        c[0][0] = 0;
        main2 (c , 5.4);
    }
    float q = 4.2;
    
    )";
    //     std::u8string myprogram2 = u8R"(
    // int main() {
    //     int a = 1;
    //     int b = 2;
    //     int c = 1 + 2 * 3 + (4 + 5) * 6;
    //     if(a+b<c) {
    //         a = a + b;
    //         b = b + a;
    //     }
    //     else
    //         c=1000;
    //     return 0;
    // }
    // )";
    auto ss2 = Lexer::scan(toU8str(myprogram2));
    for(int i= 0 ;i < ss2.size() ; i++) {
        auto q = ss2[i];
        std::cout<<"["<<toString(q.type)<<" "<<toString(q.value)<<" "<<i<<"]";
    }
    std::cout<<std::endl;
    astT.BuildSpecifiedAST(ss2);
    Semantic::ASTContentVisitor v2;
    AST::ConstantFoldingVisitor v3;
    Semantic::SymbolTableBuildVisitor v4;
    Semantic::TypingCheckVisitor v5;
    Semantic::SemanticSymbolTable tmp;
    //v2.moveSequence = true;
    astT.root->accept(v3);
    astT.root->accept(v2);
    
    v4.build(&tmp,&astT);
    tmp.printTable();
    tmp.checkTyping();
    astT.root->accept(v2);
    Semantic::ExprTypeCheckMap ExprMap;
    v5.build(&tmp,&astT,&ExprMap);

    IRContent irbase;
    EntryOperandMap entry_map;
    IR::IRGenVisitor v6;
    v6.build(&tmp,&astT,&irbase,&entry_map);
    irbase.print();
    std::cout<<"stop\n";
    return;
    
}


}

#endif