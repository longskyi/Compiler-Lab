#ifndef M_LCMP_TYPING_SYSTEM_HEADER
#define M_LCMP_TYPING_SYSTEM_HEADER

#include<memory>
#include<vector>
#include<string>
#include<optional>
#include "stringUtil.h"
#include<iostream>
#include <expected>
namespace AST {

enum baseType {
    NonInit,
    INT,
    FLOAT,
    VOID,
    FUNC,   //支持Param类型检查
    ARRAY,
    BASE_PTR,
    FUNC_PTR,
};

enum CAST_OP {
    INVALID,       // 非法转换
    NO_OP,         // 完全等效的类型（如 int → int）
    PTR_TO_PTR,    // 指针类型转换（如 int (*)[9] → int**）
    INT_TO_FLOAT,  // int -> float
    FLOAT_TO_INT,  // float -> int
    INT_TO_PTR,    // int -> T*
    PTR_TO_INT,    // T* -> int（如指针转地址）
    ARRAY_TO_PTR,  // 数组退化为指针（如 int[3] -> int*）
    FUNC_TO_PTR,   // 函数名退化为函数指针
    //WIDEN,         // 整型/浮点扩展（如 char -> int）
    //TRUNCATE,      // 整型/浮点截断（如 int -> char）
};

// 解引用错误类型
enum class DerefError {
    NotDereferenceable,  // 类型不可解引用
    NullPointer,         // 指针类型但 eType 为空
    InvalidArray,        // 数组长度<=0或eType为空
    FunctionType,        // 尝试解引用函数类型
};

using std::unique_ptr;

class SymType
{
    const size_t OS_PTR_SIZE = 8;
public:
    baseType basicType = NonInit;
    int array_len = 0;              //如果basic是array，则有该参数
    unique_ptr<SymType> eType = nullptr;    //是FUNC和FUNCPTR的RET类型 ， 是PTR的指向类型
    std::vector<unique_ptr<SymType>> TypeList;  //是FUNCPTR和FUNC的形参类型
    SymType() = default;
    inline SymType(SymType&& other) noexcept
        : basicType(other.basicType),
          array_len(other.array_len),
          eType(std::move(other.eType)),
          TypeList(std::move(other.TypeList)) {
        other.basicType = NonInit;
        other.array_len = 0;
    }
    inline SymType& operator=(SymType&& other) noexcept {
        if (this != &other) {
            basicType = other.basicType;
            array_len = other.array_len;
            eType = std::move(other.eType);
            TypeList = std::move(other.TypeList);

            other.basicType = NonInit;
            other.array_len = 0;
        }
        return *this;
    }
    inline SymType(const SymType& other) 
        : basicType(other.basicType),
          array_len(other.array_len) {
        // 深拷贝 eType
        if (other.eType) {
            eType = std::make_unique<SymType>(*other.eType);
        }
        
        // 深拷贝 TypeList 中的每个元素
        TypeList.reserve(other.TypeList.size());
        for (const auto& item : other.TypeList) {
            TypeList.push_back(std::make_unique<SymType>(*item));
        }
    }
    // 深拷贝赋值运算符
    inline SymType& operator=(const SymType& other) {
        if (this != &other) {
            // 拷贝基本成员
            basicType = other.basicType;
            array_len = other.array_len;
            
            // 深拷贝 eType
            if (other.eType) {
                eType = std::make_unique<SymType>(*other.eType);
            } else {
                eType.reset();
            }
            
            // 深拷贝 TypeList
            TypeList.clear();
            TypeList.reserve(other.TypeList.size());
            for (const auto& item : other.TypeList) {
                TypeList.push_back(std::make_unique<SymType>(*item));
            }
        }
        return *this;
    }
    inline size_t sizeoff() const {
        switch (basicType) {
            case INT:
                return 4;  // int32_t
            case FLOAT:
                return 4;  // 32位单精度浮点
            case VOID:
                return 0;  // void类型不占空间
            case BASE_PTR:
            case FUNC_PTR:
                return 8;  // 64位指针
            case ARRAY:
                if (eType && array_len > 0) {
                    return eType->sizeoff() * array_len;  // 递归计算数组元素大小
                }
                return 0;  // 无效数组
            case FUNC:
                return 0;  // 函数类型本身不占空间（函数指针才占空间）
            case NonInit:
            default:
                return 0;  // 未初始化类型
        }
    }
    inline size_t alignmentof() const {
        switch (basicType) {
            case INT:
                return 4;  // int32_t
            case FLOAT:
                return 4;  // 32位单精度浮点
            case VOID:
                return 0;  // void类型不占空间
            case BASE_PTR:
            case FUNC_PTR:
                return OS_PTR_SIZE;  // 64位指针
            case ARRAY:
                if (eType && array_len > 0) {
                    return eType->alignmentof();  // 递归计算数组元素大小
                }
                return 0;  // 无效数组
            case FUNC:
                return 0;  // 函数类型本身不占空间（函数指针才占空间）
            case NonInit:
            default:
                return 0;  // 未初始化类型
        }
    }
    //获取指针+1的步长（字节数）
    inline size_t pointerStride() const {
        if (basicType == BASE_PTR || basicType == FUNC_PTR) {
            // 指针类型：步长由指向的类型决定
            if (eType) {
                return eType->sizeoff();  // 指向类型的大小
            }
            return 0;  // 无效指针类型
        }
        else if (basicType == ARRAY) {
            // 数组类型：退化为指针时的步长是元素大小
            if (eType) {
                return eType->sizeoff();
            }
            return 0;  // 无效数组类型
        }
        else {
            // 其他类型（INT/FLOAT等）的指针步长就是类型本身大小
            return sizeoff();
        }
    }
    // tuple [是否是左值,解引用后类型]
    // auto result = sym.deref();
    // if (result) {
    //     auto [is_lvalue , type] = result.value();
    //     if (is_lvalue) {
    //         // 这个解引用在 C 语言中是左值（可赋值）
    //     }
    // }
    inline std::expected<std::tuple<bool,SymType>, DerefError> deref() const {
        switch (basicType) {
            // 基础指针类型（BASE_PTR）
            case BASE_PTR: {
                if (!eType) {
                    return std::unexpected(DerefError::NullPointer);
                }
                return std::make_tuple(true,SymType(*eType));  // 返回指向的类型
            }
            // 数组类型（ARRAY）
            case ARRAY: {
                if (!eType || array_len <= 0) {
                    return std::unexpected(DerefError::InvalidArray);
                }
                return std::make_tuple(true,SymType(*eType));  // 返回数组元素类型
            }
            // 函数指针类型（FUNC_PTR）
            case FUNC_PTR: {
                // 函数指针的解引用行为：
                // 1. 返回函数类型（FUNC）
                // 2. 保留相同的返回类型和参数列表
                if (!eType) {
                    return std::unexpected(DerefError::NullPointer);
                }
                
                SymType result;
                result.basicType = FUNC;
                result.eType = std::make_unique<SymType>(*eType);  // 拷贝返回类型
                
                // 拷贝参数列表
                for (const auto& param : TypeList) {
                    result.TypeList.push_back(std::make_unique<SymType>(*param));
                }
                
                return std::make_tuple(false,result);
            }

            // 函数类型（FUNC）本身不可解引用
            case FUNC:
                return std::unexpected(DerefError::FunctionType);

            // 其他不可解引用的类型
            case INT:
            case FLOAT:
            case VOID:
            case NonInit:
            default:
                return std::unexpected(DerefError::NotDereferenceable);
        }
    }
    inline bool check() const {
        switch (basicType) {
            case NonInit:
                return false;  // 未初始化的类型不合法

            case INT:
            case FLOAT:
            case VOID:
                // 基本类型不能有 eType 或 TypeList 或 array_len
                return (eType == nullptr) && TypeList.empty() && (array_len == 0);

            case BASE_PTR:
                // 基础指针必须有 eType 且 eType 本身合法
                // 不能有 TypeList 或 array_len
                return (eType != nullptr) && eType->check() && 
                    TypeList.empty() && (array_len == 0);

            case FUNC_PTR:
                // 函数指针必须有 eType (返回类型) 且合法
                // TypeList 中的每个参数类型必须合法
                // 不能有 array_len
                if (eType == nullptr || !eType->check() || array_len != 0) {
                    return false;
                }
                for (const auto& param : TypeList) {
                    if (param == nullptr || !param->check()) {
                        return false;
                    }
                }
                return true;

            case FUNC:
                // 函数类型必须有 eType (返回类型) 且合法
                // TypeList 中的每个参数类型必须合法
                // 不能有 array_len
                if (eType == nullptr || !eType->check() || array_len != 0) {
                    return false;
                }
                for (const auto& param : TypeList) {
                    if (param == nullptr || !param->check()) {
                        return false;
                    }
                }
                return true;

            case ARRAY:
                // 数组类型必须有 eType (元素类型) 且合法
                // array_len 必须 > 0
                // 不能有 TypeList
                return (eType != nullptr) && eType->check() && 
                    (array_len > 0) && TypeList.empty();

            default:
                return false;  // 未知类型不合法
        }
    }
    inline ~SymType(){}
    inline void makePtr() {
        auto NewEtype = std::make_unique<SymType>(std::move(*this));
        this->eType = std::move(NewEtype);
        this->TypeList.clear();
        if(this->eType->basicType == FUNC) {
            this->basicType = FUNC_PTR;
        }   
        else {
            this->basicType = BASE_PTR;
        }
    }
    inline void makeFuncPtr(const std::vector<SymType >& TypeList_) {
        auto NewEtype = std::make_unique<SymType>(std::move(*this));
        this->eType = std::move(NewEtype);
        this->TypeList.clear();
        for(auto & st : TypeList_) {
            this->TypeList.emplace_back(std::make_unique<SymType>(st));
        }
        this->basicType = FUNC_PTR;
    }
    inline void makeFunc(const std::vector<SymType> & TypeList_) {
        auto NewEtype = std::make_unique<SymType>(std::move(*this));
        this->eType = std::move(NewEtype);
        this->TypeList.clear();
        for(auto & st : TypeList_) {
            this->TypeList.emplace_back(std::make_unique<SymType>(st));
        }
        this->basicType = FUNC;
    }
    inline void makeArray(int len) {
        auto NewEtype = std::make_unique<SymType>(std::move(*this));
        this->eType = std::move(NewEtype);
        this->TypeList.clear();
        this->basicType = ARRAY;
        this->array_len = len;
    }
    inline static bool equals(const SymType& a, const SymType& b) {
        if (a.basicType != b.basicType) {
            return false;
        }
        // 对于指针、数组和函数指针类型，需要递归比较指向的类型
        if (a.basicType == BASE_PTR || a.basicType == ARRAY || a.basicType == FUNC_PTR) {
            if(a.array_len!=b.array_len) {
                return false;
            }
            if ((a.eType == nullptr) != (b.eType == nullptr)) {
                return false;
            }
            if (a.eType && !equals(*a.eType, *b.eType)) {
                return false;
            }
        }
        // 对于函数指针类型，还需要比较参数类型列表
        if (a.basicType == FUNC_PTR || a.basicType == FUNC) {
            if(a.basicType != b.basicType) return false;
            if (a.TypeList.size() != b.TypeList.size()) {
                return false;
            }
            if(!equals(*(a.eType.get()),*(b.eType.get()))) {
                //检查函数返回值
                return false;
            }
            for (size_t i = 0; i < a.TypeList.size(); ++i) {
                if (!equals(*a.TypeList[i], *b.TypeList[i])) {
                    return false;
                }
            }
        }
        return true;
    }
    inline static std::u8string_view format(DerefError err) {
        switch (err) {
            case DerefError::NotDereferenceable: return u8"类型不可解引用";
            case DerefError::NullPointer: return u8"指针类型但指向类型为空";
            case DerefError::InvalidArray: return u8"无效数组类型";
            case DerefError::FunctionType: return u8"函数类型不可解引用";
            default: return u8"未知错误";
        }
    }
    inline std::u8string format() const {
        switch (basicType) {
            case INT: return u8"int";
            case FLOAT: return u8"float";
            case VOID: return u8"void";
            //case CHAR: return u8"char";
            
            case BASE_PTR: {
                if (!eType) return u8"void*";
                std::u8string inner = eType->format();
                // 处理函数指针的特殊情况
                if (eType->basicType == FUNC || eType->basicType == FUNC_PTR) {
                    return inner;  // 函数指针会在函数类型中处理
                }
                return inner + u8"*";
            }
            
            case ARRAY: {
                if (!eType) return u8"void[" + toU8str(std::to_string(array_len)) + u8"]";
                std::u8string inner = eType->format();
                // 处理数组指针的特殊情况
                if (eType->basicType == FUNC || eType->basicType == FUNC_PTR) {
                    return u8"(" + inner + u8")[" + toU8str(std::to_string(array_len)) + u8"]";
                }
                return u8"[" + toU8str(std::to_string(array_len)) + u8"]" + inner;
            }
            case FUNC:
            case FUNC_PTR: {
                std::u8string result;
                // 处理返回类型
                if (eType) {
                    result = eType->format();
                } else {
                    result = u8"void";
                }
                // 添加指针标记（如果是函数指针）
                if (basicType == FUNC_PTR) {
                    result += u8"(*)";
                } else {
                    result += u8" ";
                }
                // 添加参数列表
                result += u8"(";
                for (size_t i = 0; i < TypeList.size(); ++i) {
                    if (i > 0) result += u8", ";
                    result += TypeList[i]->format();
                }
                result += u8")";
                
                return result;
            }
            
            default:
                return u8"unknown";
        }
    }
    inline std::tuple<CAST_OP, std::u8string> cast_to(const SymType* target) const {
        // 1. 检查空指针
        if (target == nullptr) {
            return {INVALID, u8"内部错误，目标类型为空指针"};
        }
        // 2. 相同类型可以直接转换（无警告）
        if (equals(*this, *target)) {
            return {NO_OP, u8""};
        }
        // 3. void 指针的特殊处理（修正版）
        if (basicType == BASE_PTR && target->basicType == BASE_PTR) {
            // 先确保双方都是指针类型
            if (eType && eType->basicType == VOID) {
                return {PTR_TO_PTR, u8""};  // void* 可以转换为任何指针类型
            }
            if (target->eType && target->eType->basicType == VOID) {
                return {PTR_TO_PTR, u8""};  // 任何指针类型可以转换为 void*
            }
            // 非void指针之间的转换（可能有警告）
            return {PTR_TO_PTR, u8"不同类型的指针转换可能有风险"};
        }
        //PTR TO INT情况
        if (basicType == BASE_PTR && target->basicType == INT) {
            return {PTR_TO_INT, u8"指针到整型的隐式转换（可能不安全）"};
        }
        // 4. 数值类型转换检查
        if ((basicType == INT || basicType == FLOAT) && 
            (target->basicType == INT || target->basicType == FLOAT)) {
            // 数值类型之间可以互相转换，但可能有精度损失警告
            if (basicType == FLOAT && target->basicType == INT) {
                return {FLOAT_TO_INT, u8"浮点数转换为整数可能导致精度损失"};
            }
            if (basicType == INT && target->basicType == FLOAT) {
                return {INT_TO_FLOAT, u8""};
            }
            return {NO_OP, u8""};
        }

        // 5. 数组和指针的转换
        if (basicType == ARRAY && target->basicType == BASE_PTR) {
            // 数组退化为指针，需要检查元素类型是否匹配
            if (eType && target->eType) {
                return (*eType).cast_to(target->eType.get()); //数组退化需要递归检查
                //return {NO_OP, u8""};  // 类型匹配，无警告
            }
            return {INVALID, u8"数组元素类型与指针指向类型不匹配"};
        }

        // 6. 函数指针转换
        if ((basicType == FUNC_PTR || basicType == FUNC) && 
            (target->basicType == FUNC_PTR || target->basicType == FUNC)) 
        {
            // 检查返回值类型是否兼容
            if (!eType || !target->eType) {
                return {INVALID, u8"函数返回值类型缺失"};
            }
            
            if (!equals(*eType, *target->eType)) {  // 返回值必须完全匹配
                return {INVALID, u8"函数返回值类型不匹配"};
            }

            // 宽松的形参检查规则：
            // - 如果目标函数没有参数（TypeList为空），允许任何函数指针转换（如可变参数函数）
            // - 否则，形参数量和类型必须严格匹配
            if (target->TypeList.empty()) {
                if(basicType == FUNC)
                    return {FUNC_TO_PTR, u8"目标函数接受任意参数（如可变参数函数）"};
                else
                    return {PTR_TO_PTR, u8"目标函数接受任意参数（如可变参数函数）"};
            }
            
            // 检查参数
            if (TypeList.size() != target->TypeList.size()) {
                return {INVALID, u8"形参数量不匹配"};
            }
            for (size_t i = 0; i < TypeList.size(); ++i) {
                if (!equals(*TypeList[i], *target->TypeList[i])) {  // 参数类型必须完全匹配
                    return {INVALID, toU8str(std::format("第{}个形参类型不匹配",i+1))};
                }
            }

            // 所有检查通过，允许转换（可能有警告）
            if(basicType == FUNC)
                return {FUNC_TO_PTR, u8""};
            else
                return {PTR_TO_PTR, u8""};
        }

        // 如果以上条件都不满足，转换失败
        return {INVALID, u8"类型不兼容，无法安全转换"};
    }
};


inline void test_typeSystem_main() {

    SymType intType;
    intType.basicType = INT;
    SymType floatType;
    floatType.basicType = FLOAT;

    SymType intPtrType;
    intPtrType.basicType = BASE_PTR;
    intPtrType.eType = std::make_unique<SymType>(intType);

    SymType intPtrPtrType;
    intPtrPtrType.basicType = BASE_PTR;
    intPtrPtrType.eType = std::make_unique<SymType>(intPtrType);

    SymType arrayType;
    arrayType.basicType = ARRAY;
    arrayType.array_len = 3;
    arrayType.eType = std::make_unique<SymType>(intType);
    SymType array2Type(arrayType);
    array2Type.makeArray(6);

    SymType funcType;
    funcType.basicType = FUNC;
    funcType.eType = std::make_unique<SymType>(intType);
    funcType.TypeList.push_back(std::make_unique<SymType>(intType));
    funcType.TypeList.push_back(std::make_unique<SymType>(floatType));

    SymType funcPtrType;
    funcPtrType.basicType = FUNC_PTR;
    funcPtrType.eType = std::make_unique<SymType>(intType);
    funcPtrType.TypeList.push_back(std::make_unique<SymType>(intType));
    funcPtrType.TypeList.push_back(std::make_unique<SymType>(floatType));

    // 预期输出:
    std::cout<<toString_view(intType.format());       // "int"
    std::cout<<std::endl;
    std::cout<<toString_view(intPtrType.format());    // "int*"
    std::cout<<std::endl;
    std::cout<<toString_view(intPtrPtrType.format()); // "int**"
    std::cout<<std::endl;
    std::cout<<toString_view(arrayType.format());     // "int[3]"
    std::cout<<std::endl;
    std::cout<<toString_view(array2Type.format());     // "int[3]"
    std::cout<<std::endl;
    arrayType.makePtr();
    array2Type.makePtr();
    std::cout<<toString_view(arrayType.format());     // "int(*)[3]"
    std::cout<<std::endl;
    std::cout<<toString_view(array2Type.format());     // "int(*)[3]"
    std::cout<<std::endl;
    std::cout<<toString_view(funcType.format());      // "int (int, float)"
    std::cout<<std::endl;
    std::cout<<toString_view(funcPtrType.format());   // "int(*)(int, float)"
    std::cout<<std::endl;
}

}


#endif