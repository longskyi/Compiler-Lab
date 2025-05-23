#include "Semantic.h"


namespace Semantic {


bool parseExprCheck(AST::Expr * mainExpr_ptr , ExprTypeCheckMap & ExprMap) {
    if(mainExpr_ptr == nullptr) {
        std::cout<<std::format("Expr检查失败，空指针错误\n");
        return false;
    }
    if(dynamic_cast<AST::ConstExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::ConstExpr *>(mainExpr_ptr);
        TypingCheckNode currNode;
        currNode.retType = Expr_ptr->Type;
        ExprMap[Expr_ptr] = std::move(currNode);
    }
    if(dynamic_cast<AST::DerefExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::DerefExpr *>(mainExpr_ptr);
        auto subExpr_ptr = Expr_ptr->subExpr.get();
        auto subTyping = ExprMap.at(subExpr_ptr);
        auto ret = subTyping.retType.deref();
        if(!ret) {
            std::cerr<<std::format("对类型{}的解引用失败\n",toString_view(subTyping.retType.format()));
            auto Error = ret.error();
            //AST::SymType::
            return false;
        }
        auto [canBeLval,Symstype] = ret.value();
        //如果被解引用后成了左值，那么被解引用前被视为右值
        ExprMap.at(subExpr_ptr).ret_is_left_value = ExprMap.at(subExpr_ptr).ret_is_left_value && !canBeLval;
        TypingCheckNode currNode;
        currNode.retType = std::move(Symstype);
        currNode.ret_is_left_value = canBeLval;
        ExprMap[Expr_ptr] = std::move(currNode);
    }
    if(dynamic_cast<AST::CallExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::CallExpr *>(mainExpr_ptr);
        SymbolEntry * Se_ptr = static_cast<SymbolEntry *>(Expr_ptr->id_ptr->symEntryPtr);
        if(Se_ptr == nullptr) {
            std::cerr<<"内部错误，符号没有被绑定"<<std::endl;
            return false;
        }
        const auto & idType = Se_ptr->Type;
        if(idType.basicType != AST::baseType::FUNC && idType.basicType != AST::baseType::FUNC_PTR) {
            std::cerr<<
            std::format("错误，变量{} 类型{}不是函数类型",
                toString_view(Se_ptr->symbolName),toString_view(idType.format()));
            return false;
        }
        /**
        
        重要：此处只实现 Param -> Expr支持，足以覆盖原有文法
        
        */
        const auto & paramL = Expr_ptr->ParamList_ptr->paramList;
        if(paramL.size() != idType.TypeList.size() && idType.TypeList.size()!=0) {
            std::cerr<<std::format("函数{}实参形参数量不匹配",toString_view(Se_ptr->symbolName));
            return false;
        }

        for(int i = 0 ; i< paramL.size() && idType.TypeList.size()!=0 ;i++) {
            // Expr -> id( paramLists ) -> param -> Expr
            const auto & paramT = paramL[i];
            const auto & argType = idType.TypeList[i].get();
            const auto paramExpr_ptr = paramT->expr_ptr.get();
            //忽略 id_ptr 和 Op
            auto & paramTypeNode = ExprMap.at(paramExpr_ptr);
            auto [cast_op,msg] = paramTypeNode.retType.cast_to(argType);
            if(cast_op == AST::CAST_OP::INVALID) {
                std::cerr<<std::format("函数调用，类型转换失败，源类型{}，无法转换成{}\n",
                    toString_view(paramTypeNode.retType.format()),
                    toString_view(argType->format())
                );
                return false;
            }
            else {
                if(!msg.empty()) {
                    std::cerr<<"类型转换警告："<<toString_view(msg)<<std::endl;
                }
                paramTypeNode.castType = AST::SymType(*argType);
                paramTypeNode.cast_op = cast_op;
            }
        }
        TypingCheckNode currNode;
        currNode.retType = *(idType.eType.get());
        currNode.ret_is_left_value = false;
        ExprMap[Expr_ptr] = std::move(currNode);
    }
    if(dynamic_cast<AST::ArithExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::ArithExpr *>(mainExpr_ptr);
        /**
            INVALID,
            ADDI,
            ADDF,
            MULI,
            MULF,
            PTR_ADDI,   //必须是PTR + int，修改 helpNum1为sizeoff
            ADDU,   //暂时不用
            //指针加法不被允许，但是指针减法是存在的，暂时没有实现
        */
        auto Lexpr_ptr = Expr_ptr->Lval_ptr.get();
        auto Rexpr_ptr =Expr_ptr->Rval_ptr.get();
        auto & Lexpr_Typing_Node = ExprMap.at(Lexpr_ptr);
        auto & Rexpr_Typing_Node = ExprMap.at(Rexpr_ptr);
        const auto & Ltype = Lexpr_Typing_Node.retType;
        const auto & Rtype = Rexpr_Typing_Node.retType;
        if(Expr_ptr->Op == AST::ADD) 
        {
            if(Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::INT) {
            //ADDI
            TypingCheckNode currNode;
            currNode.retType = Lexpr_Typing_Node.retType;
            currNode.ret_is_left_value = false;
            currNode.arithOp = ADDI;
            ExprMap[Expr_ptr] = std::move(currNode);
            }
            else if( (Ltype.basicType == AST::baseType::FLOAT && Rtype.basicType == AST::baseType::INT) ||
                    (Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::FLOAT) || 
                    (Ltype.basicType == AST::baseType::FLOAT && Rtype.basicType == AST::baseType::FLOAT)) 
            {
                //ADDF
                TypingCheckNode currNode;
                currNode.retType = Lexpr_Typing_Node.retType;
                currNode.ret_is_left_value = false;
                currNode.arithOp = ADDF;
                AST::SymType floatType;
                floatType.basicType = AST::baseType::FLOAT;
                ExprMap[Expr_ptr] = std::move(currNode);
                if(Lexpr_Typing_Node.retType.basicType == AST::baseType::INT) {
                    Lexpr_Typing_Node.cast_op = AST::CAST_OP::INT_TO_FLOAT;
                    Lexpr_Typing_Node.castType = floatType;
                }
                if(Rexpr_Typing_Node.retType.basicType == AST::baseType::INT) {
                    Rexpr_Typing_Node.cast_op = AST::CAST_OP::INT_TO_FLOAT;
                    Rexpr_Typing_Node.castType = floatType;
                }
            }
            else if((Ltype.basicType == AST::baseType::BASE_PTR && Rtype.basicType == AST::baseType::INT) ||
                    (Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::BASE_PTR)) {
                //PTR ADDI
                TypingCheckNode * ptrExprTyping;
                TypingCheckNode * intExprTyping;
                if(Ltype.basicType == AST::baseType::BASE_PTR) {
                    ptrExprTyping = &Lexpr_Typing_Node;
                    intExprTyping = &Rexpr_Typing_Node;
                } else {
                    ptrExprTyping = &Rexpr_Typing_Node;
                    intExprTyping = &Lexpr_Typing_Node;                    
                }
                int ptrStepLen = ptrExprTyping->retType.pointerStride();
                if(ptrStepLen == 0 ){
                    std::cerr<<std::format("指针类型 {} + 运算不被允许\n",
                    toString_view(ptrExprTyping->retType.format()));
                    return false;
                }
                else {
                    TypingCheckNode currNode;
                    currNode.retType = ptrExprTyping->retType;
                    currNode.ret_is_left_value = false;
                    currNode.arithOp = PTR_ADDI;
                    currNode.helpNum1 = ptrStepLen;
                    ExprMap[Expr_ptr] = std::move(currNode);
                }
            }else if((Ltype.basicType == AST::baseType::ARRAY && Rtype.basicType == AST::baseType::INT) ||
                    (Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::ARRAY)) {
                //PTR ADDI
                TypingCheckNode * ptrExprTyping;
                TypingCheckNode * intExprTyping;
                if(Ltype.basicType == AST::baseType::ARRAY) {
                    ptrExprTyping = &Lexpr_Typing_Node;
                    intExprTyping = &Rexpr_Typing_Node;
                } else {
                    ptrExprTyping = &Rexpr_Typing_Node;
                    intExprTyping = &Lexpr_Typing_Node;                    
                }
                ptrExprTyping->castType = ptrExprTyping->retType;
                ptrExprTyping->cast_op = AST::ARRAY_TO_PTR;
                ptrExprTyping->castType.basicType = AST::BASE_PTR;
                ptrExprTyping->castType.array_len = 0;
                int ptrStepLen = ptrExprTyping->retType.pointerStride();
                if(ptrStepLen == 0 ){
                    std::cerr<<std::format("指针类型 {} + 运算不被允许\n",
                    toString_view(ptrExprTyping->retType.format()));
                    return false;
                }
                else {
                    TypingCheckNode currNode;
                    currNode.retType = ptrExprTyping->retType;
                    currNode.ret_is_left_value = false;
                    currNode.arithOp = PTR_ADDI;
                    currNode.helpNum1 = ptrStepLen;
                    ExprMap[Expr_ptr] = std::move(currNode);
                }
            } else {
                std::cerr<<std::format("类型 {} [+] {} 不支持 + 运算符\n",
                    toString_view(Ltype.format()),toString_view(Rtype.format()));
                return false;
            }
        } else if(Expr_ptr->Op == AST::MUL) {
            if(Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::INT) {
                // MULI
                TypingCheckNode currNode;
                currNode.retType = Lexpr_Typing_Node.retType;
                currNode.ret_is_left_value = false;
                currNode.arithOp = MULI;
                ExprMap[Expr_ptr] = std::move(currNode);
            }
            else if( (Ltype.basicType == AST::baseType::FLOAT && Rtype.basicType == AST::baseType::INT) ||
                    (Ltype.basicType == AST::baseType::INT && Rtype.basicType == AST::baseType::FLOAT) || 
                    (Ltype.basicType == AST::baseType::FLOAT && Rtype.basicType == AST::baseType::FLOAT)) 
            {
                // MULF
                TypingCheckNode currNode;
                currNode.retType.basicType = AST::baseType::FLOAT;
                currNode.ret_is_left_value = false;
                currNode.arithOp = MULF;
                AST::SymType floatType;
                floatType.basicType = AST::baseType::FLOAT;
                ExprMap[Expr_ptr] = std::move(currNode);
                if(Lexpr_Typing_Node.retType.basicType == AST::baseType::INT) {
                    Lexpr_Typing_Node.cast_op = AST::CAST_OP::INT_TO_FLOAT;
                    Lexpr_Typing_Node.castType = floatType;
                }
                if(Rexpr_Typing_Node.retType.basicType == AST::baseType::INT) {
                    Rexpr_Typing_Node.cast_op = AST::CAST_OP::INT_TO_FLOAT;
                    Rexpr_Typing_Node.castType = floatType;
                }
            } else {
                std::cerr<<std::format("类型 {} [*] {} 不支持 * 运算符\n",
                    toString_view(Ltype.format()),toString_view(Rtype.format()));
                return false;
            }
        } else {
            std::cerr<<"运算符Not implement"<<std::endl;
            return false;
        }
        //新增修改，对于Arith，要求底下都变成right value
        Lexpr_Typing_Node.ret_is_left_value = false;
        Rexpr_Typing_Node.ret_is_left_value = false;
        //
        
    }
    if(dynamic_cast<AST::IdValueExpr *>(mainExpr_ptr)) {
        auto Expr_ptr = static_cast<AST::IdValueExpr *>(mainExpr_ptr);
        auto behave = Expr_ptr->behave;
        auto id_ptr = Expr_ptr->id_ptr.get();
        auto SE_ptr = static_cast<SymbolEntry *> (id_ptr->symEntryPtr);
        if(behave == AST::direct) {
            //直接访问
            //要尽可能保留类型，直到更上层的隐式类型转换
            auto & idType = SE_ptr->Type;
            TypingCheckNode currNode;
            currNode.retType = idType;
            if(idType.basicType == AST::FUNC || idType.basicType == AST::FUNC_PTR || idType.basicType == AST::ARRAY) {
                currNode.ret_is_left_value = false;
            } else {
                currNode.ret_is_left_value = true;
            }
            ExprMap[Expr_ptr] = std::move(currNode);
        } else if(behave == AST::ref) {
            //取地址
            auto & idType = SE_ptr->Type;
            TypingCheckNode currNode;
            currNode.retType = idType;
            currNode.retType.makePtr();
            currNode.ret_is_left_value = true;
            ExprMap[Expr_ptr] = std::move(currNode);
        } else {
            std::cerr<<"内部错误，Id behave未初始化"<<std::endl;
            return false;
        }
        
    }

    return true;
}

//不包含return检查
bool parseStmtCheck(AST::Stmt * mainStmt_ptr, ExprTypeCheckMap & ExprMap) {
    if(mainStmt_ptr == nullptr) {
        std::cout<<std::format("Stmt检查失败，空指针错误\n");
        return false;
    }
    if(dynamic_cast<AST::FunctionCall *>(mainStmt_ptr)) {
        auto Stmt_ptr = static_cast<AST::FunctionCall *>(mainStmt_ptr);
        SymbolEntry * Se_ptr = static_cast<SymbolEntry *>(Stmt_ptr->id_ptr->symEntryPtr);
        if(Se_ptr == nullptr) {
            std::cerr<<"内部错误，符号没有被绑定"<<std::endl;
            return false;
        }
        const auto & idType = Se_ptr->Type;
        if(idType.basicType != AST::baseType::FUNC && idType.basicType != AST::baseType::FUNC_PTR) {
            std::cerr<<
            std::format("错误：变量{} 类型{}不是函数类型\n",
                toString_view(Se_ptr->symbolName),toString_view(idType.format()));
            return false;
        }
        if(idType.eType->basicType != AST::baseType::VOID) {
            std::cerr<<"警告："<<toString_view(Se_ptr->symbolName)<<"函数返回值未被使用\n";
        }
        /**
        
        重要：此处只实现 Param -> Expr支持，足以覆盖原有文法
        
        */
        const auto & paramL = Stmt_ptr->paramList_ptr->paramList;
        if(paramL.size() != idType.TypeList.size()) {
            std::cerr<<std::format("函数{}实参形参数量不匹配",toString_view(Se_ptr->symbolName));
            return false;
        }

        for(int i = 0 ; i< paramL.size();i++) {
            const auto & paramT = paramL[i];
            const auto & argType = idType.TypeList[i].get();
            const auto paramExpr_ptr = paramT->expr_ptr.get();
            //忽略 id_ptr 和 Op

            ExprMap.at(paramExpr_ptr).ret_is_left_value = false;

            auto & paramTypeNode = ExprMap.at(paramExpr_ptr);
            auto [cast_op,msg] = paramTypeNode.retType.cast_to(argType);
            if(cast_op == AST::CAST_OP::INVALID) {
                std::cerr<<std::format("函数调用，类型转换失败，源类型{}，无法转换成{}\n",
                    toString_view(paramTypeNode.retType.format()),
                    toString_view(argType->format())
                );
                return false;
            }
            else {
                paramTypeNode.castType = AST::SymType(*argType);
                paramTypeNode.cast_op = cast_op;
            }
        }
    }
    if(dynamic_cast<AST::Assign *>(mainStmt_ptr)) {
        auto Stmt_ptr = static_cast<AST::Assign *>(mainStmt_ptr);
        auto RNode = ExprMap.at(Stmt_ptr->Rexpr_ptr.get());
        if(Stmt_ptr->Lexpr_ptr) {
            auto LNode = ExprMap.at(Stmt_ptr->Lexpr_ptr.get());
            if(!LNode.ret_is_left_value) {
                std::cerr<<"=左侧不能是右值"<<std::endl;
                return false;
            }
            RNode.ret_is_left_value = false;
            if(AST::SymType::equals(LNode.retType,RNode.retType)) {

            }
            else {
                auto [cast_op , msg] = RNode.retType.cast_to(&LNode.retType);
                if(cast_op == AST::CAST_OP::INVALID) {
                    std::cerr<<std::format("assign，类型转换失败，源类型{}，无法转换成{}\n",
                        toString_view(RNode.retType.format()),
                        toString_view(LNode.retType.format())
                    );
                    return false;
                }
                else {
                    if(!msg.empty()) {
                        std::cerr<<"类型转换警告："<<toString_view(msg)<<std::endl;
                    }
                    RNode.castType = AST::SymType(LNode.retType);
                    RNode.cast_op = cast_op;
                }
            }
        } else if(Stmt_ptr->id_ptr) {
            auto Se_ptr =  static_cast<SymbolEntry*>(Stmt_ptr->id_ptr->symEntryPtr) ;
            auto Ltype = Se_ptr->Type;
            bool L_is_left_value = true;
            if(Ltype.basicType == AST::FUNC || Ltype.basicType == AST::ARRAY) {
                L_is_left_value = false;
            }
            if(!L_is_left_value) {
                std::cerr<<"=左侧不能是右值"<<std::endl;
                return false;
            }
            RNode.ret_is_left_value = false;
            if(AST::SymType::equals(Ltype,RNode.retType)) {

            }
            else {
                auto [cast_op , msg] = RNode.retType.cast_to(&Ltype);
                if(cast_op == AST::CAST_OP::INVALID) {
                    std::cerr<<std::format("assign，类型转换失败，源类型{}，无法转换成{}\n",
                        toString_view(RNode.retType.format()),
                        toString_view(Ltype.format())
                    );
                    return false;
                }
                else {
                    if(!msg.empty()) {
                        std::cerr<<"类型转换警告："<<toString_view(msg)<<std::endl;
                    }
                    RNode.castType = AST::SymType(Ltype);
                    RNode.cast_op = cast_op;
                }
            }
        } else {
            std::cerr<<"内部错误，受损的Assign结点";
            return false;
        }
    }
    if(dynamic_cast<AST::StmtPrint *>(mainStmt_ptr)) {
        auto Stmt_ptr = static_cast<AST::StmtPrint *>(mainStmt_ptr);
        auto RNode = ExprMap.at(Stmt_ptr->expr_ptr.get());
        if(RNode.retType.basicType != AST::baseType::INT) {
            std::cerr<<"目前print只支持打印INT类型\n";
            return false;
        }
        RNode.ret_is_left_value = false;
        return true;
    }
    return true;
}

bool parseBoolCheck(AST::ASTBool * ASTbool_ptr , ExprTypeCheckMap & ExprMap) {
    if(ASTbool_ptr == nullptr) {
        std::cout<<std::format("Bool检查失败，空指针错误\n");
        return false;
    }
    
    auto Lval = ASTbool_ptr->Lval_ptr.get();
    auto& Lnode = ExprMap[Lval];
    
    // 1. 处理无比较运算符的情况（直接转换为bool）
    if(ASTbool_ptr->rop == AST::NoneROP) {
        Lnode.ret_is_left_value = false; // 作为条件表达式总是右值
        
        // 检查类型是否可转换为bool
        switch(Lnode.retType.basicType) {
            case AST::VOID:
                std::cerr << std::format("错误：void类型不能转换为bool\n");
                return false;
            case AST::ARRAY:
                // 数组退化为指针
                Lnode.castType = Lnode.retType;
                Lnode.castType.basicType = AST::BASE_PTR;
                Lnode.castType.array_len = 0;
                Lnode.cast_op = AST::ARRAY_TO_PTR;
                break;
            case AST::FUNC:
                // 函数退化为函数指针
                Lnode.castType = Lnode.retType;
                Lnode.castType.basicType = AST::FUNC_PTR;
                Lnode.cast_op = AST::FUNC_TO_PTR;
                break;
            default: // INT, FLOAT, PTR等类型可以直接作为bool
                break;
        }
        return true;
    }
    
    // 2. 处理有比较运算符的情况
    auto Rval = ASTbool_ptr->Rval_ptr.get();
    auto& Rnode = ExprMap[Rval];
    
    // 两边都不能是void
    if(Lnode.retType.basicType == AST::VOID || Rnode.retType.basicType == AST::VOID) {
        std::cerr << std::format("错误：void类型不能参与比较\n");
        return false;
    }
    
    // 处理数组退化
    if(Lnode.retType.basicType == AST::ARRAY) {
        Lnode.castType = Lnode.retType;
        Lnode.castType.basicType = AST::BASE_PTR;
        Lnode.castType.array_len = 0;
        Lnode.cast_op = AST::ARRAY_TO_PTR;
    }
    if(Rnode.retType.basicType == AST::ARRAY) {
        Rnode.castType = Rnode.retType;
        Rnode.castType.basicType = AST::BASE_PTR;
        Rnode.castType.array_len = 0;
        Rnode.cast_op = AST::ARRAY_TO_PTR;
    }
    
    // 处理函数退化
    if(Lnode.retType.basicType == AST::FUNC) {
        Lnode.castType = Lnode.retType;
        Lnode.castType.basicType = AST::FUNC_PTR;
        Lnode.cast_op = AST::FUNC_TO_PTR;
    }
    if(Rnode.retType.basicType == AST::FUNC) {
        Rnode.castType = Rnode.retType;
        Rnode.castType.basicType = AST::FUNC_PTR;
        Rnode.cast_op = AST::FUNC_TO_PTR;
    }
    
    // 获取实际比较类型（考虑可能的转换后类型）
    const auto& Ltype = (Lnode.cast_op != AST::NO_OP) ? Lnode.castType : Lnode.retType;
    const auto& Rtype = (Rnode.cast_op != AST::NO_OP) ? Rnode.castType : Rnode.retType;
    
    // 检查类型是否可比较
    if(Ltype.basicType == AST::FLOAT || Rtype.basicType == AST::FLOAT) {
        // 涉及float的比较
        if(Ltype.basicType != AST::FLOAT && Ltype.basicType != AST::INT) {
            std::cerr << std::format("错误：类型{}不能与float比较\n", toString_view(Ltype.format()));
            return false;
        }
        if(Rtype.basicType != AST::FLOAT && Rtype.basicType != AST::INT) {
            std::cerr << std::format("错误：类型{}不能与float比较\n", toString_view(Rtype.format()));
            return false;
        }
        
        // 需要int转float
        if(Ltype.basicType == AST::INT) {
            Lnode.castType.basicType = AST::FLOAT;
            Lnode.cast_op = AST::INT_TO_FLOAT;
        }
        if(Rtype.basicType == AST::INT) {
            Rnode.castType.basicType = AST::FLOAT;
            Rnode.cast_op = AST::INT_TO_FLOAT;
        }
    } 
    else if(Ltype.basicType == AST::BASE_PTR || Rtype.basicType == AST::BASE_PTR) {
        // 指针比较
        if(Ltype.basicType != AST::BASE_PTR || Rtype.basicType != AST::BASE_PTR) {
            std::cerr << std::format("错误：指针只能与指针比较\n");
            return false;
        }
        
        // 检查指针类型是否兼容
        if(!AST::SymType::equals(Ltype, Rtype)) {
            std::cerr << std::format("错误：不兼容的指针类型比较: {} 和 {}\n", 
                toString_view(Ltype.format()), toString_view(Rtype.format()));
            return false;
        }
    }
    else if(Ltype.basicType != Rtype.basicType) {
        std::cerr << std::format("错误：不兼容的类型比较: {} 和 {}\n", 
            toString_view(Ltype.format()), toString_view(Rtype.format()));
        return false;
    }
    
    // 标记为右值
    Lnode.ret_is_left_value = false;
    Rnode.ret_is_left_value = false;
    
    return true;
}

bool parseReturnCheck(AST::Return * return_ptr , SemanticSymbolTable * rootTable ,AST::FuncDeclare * func_ptr, ExprTypeCheckMap & ExprMap) {
    auto & funcName = func_ptr->id_ptr->Literal;
    auto Se_ptr = rootTable->lookup(funcName);
    if(Se_ptr == nullptr) {
        std::cerr<<"函数return检查失败 损坏的符号表\n";
        return false;
    }
    auto retType = *Se_ptr->Type.eType.get();
    if(!return_ptr->expr_ptr.has_value()) {
        if(retType.basicType != AST::baseType::VOID) {
            std::cerr<<"函数返回值与声明不符，不为空\n";
            return false;
        }
        return true;
    }
    else {
        auto ExprType = ExprMap.at(return_ptr->expr_ptr.value().get()).retType;
        auto [cast_op,msg] = ExprType.cast_to(&retType);
        if(cast_op == AST::CAST_OP::INVALID) {
            std::cerr<<std::format("return Expr，类型转换失败，源类型{}，无法转换成{}\n",
                toString_view(ExprType.format()),
                toString_view(retType.format())
            );
            return false;
        }
        else {
            if(!msg.empty()) {
                std::cerr<<"类型转换警告："<<toString_view(msg)<<std::endl;
            }
            ExprMap.at(return_ptr->expr_ptr.value().get()).castType = retType;
            ExprMap.at(return_ptr->expr_ptr.value().get()).cast_op = cast_op;
            return true;
        }
    }
}  


bool parseArgCheck(AST::Arg * Arg_ptr) {
    const auto & argtype = Arg_ptr->argtype;
    auto q = argtype.sizeoff();
    if(q>8) {
        std::cerr<<std::format("不合法的形参类型{}\n",toString_view(argtype.format()));
        return false;
    }
    return true;
}


} // end namespace Semantic