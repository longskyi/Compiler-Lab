#ifndef LCMP_IR_GEN_HEADER
#define LCMP_IR_GEN_HEADER
#include "AST/AST.h"
#include "Semantic.h"
#include "templateUtil.h"
#include <unordered_map>
namespace IR {

using std::u8string;

using OperandId = StrongId<struct OperandIdTag>;

const int PTR_LENGTH_IN_BYTES = 8;

enum Datatype {
        noneinit,
        i32,
        ptr,    //(i64)
        f32,
};

inline u8string IRformat(const Datatype & datatype) {
    switch (datatype) {
        case Datatype::noneinit:
            return u8"noneinit";  // 非初始化类型（特殊处理）
        case Datatype::i32:
            return u8"i32";       // 32位整数
        case Datatype::ptr:
            return u8"ptr";       // 指针类型
        case Datatype::f32:
            return u8"f32";     // 32位浮点数
        default:
            return u8"unknown";   // 未知类型（错误处理）
    }
    return u8"unknown";
}


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
        //std::cerr<<"警告：尝试获取VOID的类型\n";
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
    inline void clear() {
        unique_id = OperandId(0);
        datatype = noneinit;
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
            case OperandType::i32:  ret += u8"i@"; break;
            case OperandType::ptr:  ret += u8"p@"; break;
            case OperandType::f32:  ret += u8"f@"; break;
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
            case OperandType::i32:      ret += u8"i%";    break;
            case OperandType::ptr:      ret += u8"p%";    break;
            case OperandType::f32:      ret += u8"f%";    break;
            case OperandType::LABEL:    ret += u8"L";      break;
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
    Operand trueLabel;
    Operand falseLabel;
    bool validate() const {
        if (is_conditional) {
            if (condition.datatype != Operand::OperandType::i32) {
                return false;  // 条件必须是i32
            }
        }
        // 标签必须是LABEL类型
        if (trueLabel.datatype != Operand::OperandType::LABEL || 
            (is_conditional && falseLabel.datatype != Operand::OperandType::LABEL)) {
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
    labelInst(Operand label_) {
        label = label_;
    }
    labelInst(){}
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


inline u8string BinaryOpInstOPtypeToString(BinaryOpInst::OPtype type) {
    switch(type) {
    case BinaryOpInst::OPtype::i32: return u8"i32";
    case BinaryOpInst::OPtype::ptr: return u8"ptr";
    case BinaryOpInst::OPtype::f32: return u8"f32";
    default: return u8"";
    }
}

u8string IRformat(const IRinst inst);

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

//IR基本块结构
struct IRBaseBlock {
    Operand enterLabel;
    std::vector<Operand> successorLabel;
    std::unordered_set<IRBaseBlock *>predecessor;   //前继Block
    std::unordered_set<IRBaseBlock *>successor;     //后继Block
    std::deque<IRinst> Insts;
};

struct IROptimized {
    IRBaseBlock* enterBlock;
    std::vector<std::unique_ptr<IRBaseBlock>> BlockLists;

    void printCFG(std::ostream& out = std::cout) const {

        std::unordered_map<Operand, size_t> labelToIndex;
        for (size_t i = 0; i < BlockLists.size(); ++i) {
            labelToIndex[BlockLists[i]->enterLabel] = i;
        }
        // 2. 打印每个基本块及其后继关系
        out << "Control Flow Graph:\n";
        out << "===================\n";
        for (size_t i = 0; i < BlockLists.size(); ++i) {
            const auto& block = BlockLists[i];
            out << "BB" << i << " [" << toString_view(block->enterLabel.format()) << "]:\n";
            if (!block->Insts.empty()) {
                out << "  ";
                if (auto label = std::get_if<labelInst>(&block->Insts.front())) {
                    for (size_t j = 1; j < std::min(size_t(3), block->Insts.size()); ++j) {
                        out << toString_view(IRformat(block->Insts[j])) << "; ";
                    }
                } else {
                    for (size_t j = 0; j < std::min(size_t(3), block->Insts.size()); ++j) {
                        out << toString_view(IRformat(block->Insts[j])) << "; ";
                    }
                }
                if (block->Insts.size() > 3) out << "...";
                out << "\n";
            }
            out << "  Successors: ";
            if (block->successor.empty()) {
                out << "<none>";
            } else {
                bool first = true;
                for (const auto& succ : block->successor) {
                    if (!first) out << ", ";
                    first = false;
                    auto it = labelToIndex.find(succ->enterLabel);
                    if (it != labelToIndex.end()) {
                        out << "BB" << it->second;
                    } else {
                        out << "<?>";
                    }
                }
            }
            out << "\n\n";
        }
    }
};

struct FunctionIR {
    u8string funcName;
    Datatype retType;
    std::vector<Operand> Args;
    std::vector<IRinst> Instblock;
    std::unique_ptr<IROptimized> IRoptimized = nullptr;
    Operand FuncPos;    //ptr
};

bool splitIRblock(FunctionIR & funcIR);

bool mergeLinearBlocks(FunctionIR& funcIR);


class IRContent {
public:
    std::vector<GlobalVarIR> globalVar;
    std::vector<FunctionIR> FunctionIR;
    void print(std::ostream & out = std::cout) const {
        auto ptab = [&out](int depth = 1){
            for(int i =0 ; i<depth;i++) {
                out<<"    ";
            }
            return;
        };
        for(const auto & gb : globalVar) {
            out<<toString_view(gb.format())<<std::endl;
        }
        for(const auto & func : FunctionIR) {
            out<<std::format("define {} @{} (",
                toString_view(func.funcName),toString_view(IRformat(func.retType)));
            for(const auto & argT : func.Args) {
                out<<toString_view(IRformat(argT))<<" ";
            }
            out<<"){\n";
            for(const auto & inst : func.Instblock) {
                if(auto inst_ptr = std::get_if<labelInst>(&inst)) {
                    ptab(0);
                } else {
                    ptab(1);
                }
                out<<toString_view(IRformat(inst))<<"\n";
            }
            out<<"}\n";

            if (func.IRoptimized) {
                out << "\n";
                func.IRoptimized->printCFG(out);
            } else {
                for (const auto& inst : func.Instblock) {
                    if (auto inst_ptr = std::get_if<labelInst>(&inst)) {
                        ptab(0);
                    } else {
                        ptab(1);
                    }
                    out << toString_view(IRformat(inst)) << "\n";
                }
            }
        }
    }

};

//在AST的id Node上绑定了 SymbolEntry ptr，这里继续SymbolEntry -> Operand 映射
using EntryOperandMap = std::unordered_map<Semantic::SymbolEntry *,Operand>;

//生成IR时候需要的上下文
struct IRNodeContext {
    Operand expr_addr;
    Operand BoolTrueLabel;
    Operand BoolFalseLabel;
    Operand BoolTmpLabel1;
    Operand BoolTmpLabel2;
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
    inline bool build(Semantic::SemanticSymbolTable* symtab, AST::AbstractSyntaxTree* astT, IRContent* irbase,Semantic::ExprTypeCheckMap * expr_type_map , EntryOperandMap* entrymap);
    inline void enter(AST::ASTNode* node) override;
    inline void visit(AST::ASTNode* node) override;
    inline void quit(AST::ASTNode* node) override;
};


//获取idNode对应的Operand
Operand fetchIdAddr(AST::SymIdNode * id_ptr , IRGenVisitor * IRGenerator);

//idNode对应的数据类型
Datatype fetchIdType(AST::SymIdNode * id_ptr , IRGenVisitor * IRGenerator);

//生成cast指令，添加在insts之后，返回cast后的Operand，src不应该是立即数
Operand genCastInst(std::deque<IRinst> & insts ,AST::CAST_OP cast_op , Operand src);

//生成load指令
void genLoadInst(std::deque<IRinst> & insts ,Operand srcAddr,Operand dst);

//生成store指令
void genStoreInst(std::deque<IRinst> & insts ,Operand src,Operand dstAddr);

//生成call指令，返回dst地址（对于void，dst为空Operand）
Operand genCallInst(std::deque<IRinst> & insts ,AST::SymIdNode * func_id,IRGenVisitor * irgen,std::vector<Operand> srcAddrVec);

inline Operand::OperandType TypingSystem2OperandType(AST::SymType & Type) {
    return IRtype2OperandType(TypingSystem2IRType(Type));
}

bool parseIdInitIRGen (AST::IdDeclare * idDecl_ptr,IRGenVisitor * IRGenerator);

bool parseStmtIRGen(AST::Stmt * Stmt_ptr,IRGenVisitor * IRGenerator);


bool parseExprIRGen(AST::Expr * Expr_ptr,IRGenVisitor * IRGenerator);


// Branch,// Loop, 由于布尔表达式的存在，其生成不是完全的自底向上，需要特殊处理
bool parseControlIRGenENTER(AST::Stmt* stmt_ptr, IRGenVisitor* IRGenerator);

bool parseControlIRGenQUIT(AST::Stmt* stmt_ptr, IRGenVisitor* IRGenerator);

bool parseBoolIRGenENTER(AST::ASTBool* Bool_ptr, IRGenVisitor* IRGenerator);

bool parseBoolIRGenQUIT(AST::ASTBool* Bool_ptr, IRGenVisitor* IRGenerator);

//Block , BlockItemList 
bool parseCopyNode(AST::ASTNode * node_ptr,IRGenVisitor * IRGenerator);


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


//func_or_block_entry //返回arg operand
inline std::vector<Operand> parseFuncAlloca(FunctionIR & funcir,Semantic::SymbolEntry * func_Entry , IRGenVisitor * IRGenerator) {
    std::vector<Operand> ret;
    auto funcTable = func_Entry->subTable.get();
    auto & args = funcTable->Args;
    auto & entries = funcTable->entries;
    IR::labelInst init_label;
    init_label.label = Operand::allocLabel(funcir.funcName+u8"Init");
    if(!func_Entry->is_block)
        funcir.Instblock.push_back(init_label);
    for(auto & arg : args) {
        auto allocaInst = parseAlloca(arg.get(),IRGenerator);
        Operand argOp = Operand::allocOperand(TypingSystem2OperandType(arg->Type),u8"arg");
        std::deque<IRinst> code2;
        genStoreInst(code2,argOp,allocaInst.ret);
        funcir.Instblock.push_back(allocaInst);
        funcir.Instblock.push_back(code2.back());
        ret.push_back(argOp);
    }
    IR::labelInst start_label;
    start_label.label = Operand::allocLabel(funcir.funcName+u8"Start");
    if(!func_Entry->is_block)
        funcir.Instblock.push_back(start_label);
    for(auto & ent : entries) {
        if(ent->is_block) {
            parseFuncAlloca(funcir,ent.get(),IRGenerator);
        } else {
            auto allocaInst = parseAlloca(ent.get(),IRGenerator);
            funcir.Instblock.push_back(allocaInst);
        }
    }
    return ret;
}


inline bool IRGenVisitor::build(Semantic::SemanticSymbolTable* symtab, AST::AbstractSyntaxTree* astT, IRContent* irbase,Semantic::ExprTypeCheckMap * expr_type_map , EntryOperandMap* entrymap) {
    rootTable = symtab;
    currentTable = symtab;
    EntrySymMap = entrymap;
    ExprTypeMap = expr_type_map;
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
                return false;
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
                return false;
            }
            FunctionIR funcIR;
            funcIR.funcName = func->id_ptr->Literal;
            funcIR.retType = TypingSystem2IRType(*func->funcType.eType.get());
            //funcIR.Instblock;
            funcIR.FuncPos = Operand::allocVAR(funcIR.funcName,Operand::GLOBAL);
            auto SymEntry_ptr = static_cast<Semantic::SymbolEntry*>(func->id_ptr->symEntryPtr);
            if(EntrySymMap->count(SymEntry_ptr)) {
                std::cerr<<"函数分配内部错误\n";
                return false;
            }
            (*EntrySymMap)[SymEntry_ptr] = funcIR.FuncPos;
            curr_fucntion_ir = &funcIR;
            auto argOps = parseFuncAlloca(funcIR,SymEntry_ptr,this);
            funcIR.Args = argOps;
            funcBlock->accept(*this);
            if(!splitIRblock(funcIR)) {
                return false;
            }
            if(!mergeLinearBlocks(funcIR)) {
                return false;
            }
            IRbase->FunctionIR.emplace_back(std::move(funcIR));
            //树上遍历
        }
    }
    return true;
}

inline void IRGenVisitor::enter(AST::ASTNode* node) {
    if(gotError) return;
    ASTStack.push(node);
    if(node->Ntype == AST::ASTType::ASTBool) {
        parseBoolIRGenENTER(static_cast<AST::ASTBool *>(node),this);
    } else if(node->Ntype == AST::ASTType::Stmt) {
        parseControlIRGenENTER(static_cast<AST::Stmt *>(node),this);
    }
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
    if(node->Ntype == AST::ASTType::Expr) {
        parseExprIRGen(static_cast<AST::Expr *>(node),this);
    } else if(node->Ntype == AST::ASTType::Stmt) {
        parseStmtIRGen(static_cast<AST::Stmt *>(node),this);
        parseControlIRGenQUIT(static_cast<AST::Stmt *>(node),this);
    } else if(node->Ntype == AST::ASTType::ASTBool) {
        parseBoolIRGenQUIT(static_cast<AST::ASTBool *>(node),this);
    } else if(node->subType == AST::ASTSubType::IdDeclare) {
        parseIdInitIRGen(static_cast<AST::IdDeclare *>(node),this);
    }
    parseCopyNode(node,this);
    if(node->subType == AST::ASTSubType::Block) {
        if(ASTStack.empty()) {
            auto & context = ContextMap.at(node);
            curr_fucntion_ir->Instblock.insert(curr_fucntion_ir->Instblock.end(),
                context.code.begin(),context.code.end());
            ContextMap.erase(node);
        }
    }
}



//寄存器重命名
void renameOperandInPlace(Operand& target, const Operand& from, const Operand& to);

void renameOperand(IRinst& inst, const Operand& from, const Operand& to);

void renameOperandsInBlock(std::vector<IRinst>& block, const Operand& from, const Operand& to);

//常量传播待完成

//mem2Reg待完成



inline void test_IR_main() {
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

    // std::u8string myprogram2 = u8R"(
    // int acc =1;
    // float **** b;
    // int main2(int c [][30] ;float b; int s (int,int) ) {
    //     int a = 5;
    //     a = s(1,5);
    //     s(2,6);
    // }
    // int main(int p;float L) {
    //     int ps;
    //     ps = 5;
    //     int a = 200 * 4.11111;
    //     int * refa = &a;
    //     *refa = 4.2;
    //     a = *refa;
    //     a= 5;
    //     int q = 4;
    //     if(4 <= 3) {
    //         int b = 8;
    //         b = 12 + 20;
    //     }
    //     {
    //         int q = 20 + 14;
    //         q = 20.2;
    //         q = main(4,4.2);
    //     }
    //     q = 4.2;
    //     a = 16 * 5 + 8.9;
    //     int b = 8;
    //     int c [10][30];
    //     int ** headc = c;
    //     int * tmp = &c[10][30];
    //     c[0][0] = 0;
    //     b = main2 (c , 5.4,main);
    // }
    // float q = 4.2;
    
    // )";
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
    tmp.checkTyping();
    astT.root->accept(v2);
    Semantic::ExprTypeCheckMap ExprMap;
    v5.build(&tmp,&astT,&ExprMap);
    tmp.arrangeMem();
    tmp.printTable();
    IRContent irbase;
    EntryOperandMap entry_map;
    IR::IRGenVisitor v6;
    v6.build(&tmp,&astT,&irbase,&ExprMap,&entry_map);
    irbase.print();
    std::cout<<"stop\n";
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Function took " 
              << duration.count() << " microseconds (" 
              << duration.count() / 1000.0 << " milliseconds) to execute.\n";
    return;
    
}


}

#endif