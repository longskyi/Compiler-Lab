#include "irGen.h"

namespace IR {

bool parseIdInitIRGen (AST::IdDeclare * idDecl_ptr,IRGenVisitor * IRGenerator) {
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


bool parseStmtIRGen(AST::Stmt * Stmt_ptr,IRGenVisitor * IRGenerator) {
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
            auto Lir = Context.at(assign_ptr->Lexpr_ptr.get());
            auto Addr = Lir.expr_addr;
            auto ir = Context.at(assign_ptr->Rexpr_ptr.get());
            curr_code.insert(curr_code.end(),std::move_iterator(Lir.code.begin()),std::move_iterator(Lir.code.end()));
            curr_code.insert(curr_code.end(),std::move_iterator(ir.code.begin()),std::move_iterator(ir.code.end()));
            genStoreInst(curr_code,ir.expr_addr,Addr);
            Context[Stmt_ptr].code = std::move(curr_code);
            Context.erase(assign_ptr->Lexpr_ptr.get());
            Context.erase(assign_ptr->Rexpr_ptr.get());
        }
    }
    if(Stmt_ptr->subType == AST::ASTSubType::FunctionCall) {
        auto call_ptr = static_cast<AST::FunctionCall *>(Stmt_ptr);
        auto id_ptr = call_ptr->id_ptr.get();
        const auto & pList = call_ptr->paramList_ptr->paramList;
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
        callOpInst call_inst;
        auto func = fetchIdAddr(call_ptr->id_ptr.get(),IRGenerator);
        auto typing = static_cast<Semantic::SymbolEntry *>(call_ptr->id_ptr->symEntryPtr)->Type;
        call_inst.srcs = paramOperands;
        call_inst.targetFUNC = func; 
        Operand dst;
        call_inst.ret = dst;
        curr_code.push_back(call_inst);
        Context[Stmt_ptr].code = std::move(curr_code);
    }
    // FunctionCall, 下面有expr版本的
    if(Stmt_ptr->subType == AST::ASTSubType::Return) {
        auto return_ptr = static_cast<AST::Return *>(Stmt_ptr);
        std::deque<IRinst> curr_code;
        if(!return_ptr->expr_ptr.has_value()) {
            retInst ret;
            Context[Stmt_ptr].code.push_back(ret);
        }
        auto expr_ptr = return_ptr->expr_ptr.value().get();
        auto & ctx = Context[expr_ptr];
        retInst ret;
        ret.src = ctx.expr_addr;
        curr_code.insert(curr_code.end(),ctx.code.begin(),ctx.code.end());
        curr_code.push_back(ret);
        Context[Stmt_ptr].code = std::move(curr_code);
        Context.erase(expr_ptr);
    }
    if(Stmt_ptr->subType == AST::ASTSubType::StmtPrint) {
        auto print_ptr = static_cast<AST::StmtPrint *>(Stmt_ptr);
        std::deque<IRinst> curr_code;
        auto expr_ptr = print_ptr->expr_ptr.get();
        auto & ctx = Context[expr_ptr];
        printInst prints;
        prints.src = ctx.expr_addr;
        curr_code.insert(curr_code.end(),ctx.code.begin(),ctx.code.end());
        curr_code.push_back(prints);
        Context[Stmt_ptr].code = std::move(curr_code);
        Context.erase(expr_ptr);
    }
    // Return,
    // StmtPrint,
    return true;
}

bool parseExprIRGen(AST::Expr * Expr_ptr,IRGenVisitor * IRGenerator) {
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


        if(idExpr_ptr->behave == AST::ref ) {
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
            if(TypingNode.ret_is_left_value || TypingNode.retType.basicType == AST::ARRAY ||TypingNode.retType.basicType == AST::FUNC) {
                //左值，送变量地址 + 忽略类型转换 不生成code
                //数组 函数发生隐式类型转换
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
        auto arithExpr = static_cast<AST::ArithExpr *>(Expr_ptr);
        const auto & TypingNode = IRGenerator->ExprTypeMap->at(Expr_ptr);
        auto cast_op = TypingNode.cast_op;
        
        if(TypingNode.arithOp == Semantic::ADDI || TypingNode.arithOp == Semantic::ADDF || 
        TypingNode.arithOp == Semantic::MULI || TypingNode.arithOp == Semantic::MULF) {
            
            std::deque<IRinst> curr_code;
            bool is_float = (TypingNode.arithOp == Semantic::ADDF || TypingNode.arithOp == Semantic::MULF);
            Operand dst = Operand::allocOperand(is_float ? f32 : i32);
            
            BinaryOpInst inst;
            inst.op = (TypingNode.arithOp == Semantic::ADDI || TypingNode.arithOp == Semantic::ADDF) 
                    ? BinaryOpInst::add : BinaryOpInst::mul;
            inst.op_type = is_float ? BinaryOpInst::f32 : BinaryOpInst::i32;
            inst.dst = dst;
            inst.src1 = IRGenerator->ContextMap.at(arithExpr->Lval_ptr.get()).expr_addr;
            inst.src2 = IRGenerator->ContextMap.at(arithExpr->Rval_ptr.get()).expr_addr;
            
            auto & lcode = IRGenerator->ContextMap.at(arithExpr->Lval_ptr.get()).code;
            auto & rcode = IRGenerator->ContextMap.at(arithExpr->Rval_ptr.get()).code;
            
            curr_code.insert(curr_code.end(), std::move_iterator(lcode.begin()), std::move_iterator(lcode.end()));
            curr_code.insert(curr_code.end(), std::move_iterator(rcode.begin()), std::move_iterator(rcode.end()));
            curr_code.push_back(inst);
            
            auto truedst = genCastInst(curr_code, cast_op, dst);
            IRGenerator->ContextMap[Expr_ptr].code = std::move(curr_code);
            IRGenerator->ContextMap[Expr_ptr].expr_addr = truedst;
        } else if(TypingNode.arithOp == Semantic::PTR_ADDI) {
            std::deque<IRinst> curr_code;
            
            // 获取指针和整数的操作数
            Operand ptr_op, int_op;
            if(IRGenerator->ContextMap.at(arithExpr->Lval_ptr.get()).expr_addr.datatype == Operand::ptr) {
                ptr_op = IRGenerator->ContextMap.at(arithExpr->Lval_ptr.get()).expr_addr;
                int_op = IRGenerator->ContextMap.at(arithExpr->Rval_ptr.get()).expr_addr;
            } else {
                ptr_op = IRGenerator->ContextMap.at(arithExpr->Rval_ptr.get()).expr_addr;
                int_op = IRGenerator->ContextMap.at(arithExpr->Lval_ptr.get()).expr_addr;
            }
            
            // 获取对应的代码块
            auto & ptr_code = IRGenerator->ContextMap.at(arithExpr->Lval_ptr.get()).code;
            auto & int_code = IRGenerator->ContextMap.at(arithExpr->Rval_ptr.get()).code;
            
            // 合并代码
            curr_code.insert(curr_code.end(), std::move_iterator(ptr_code.begin()), std::move_iterator(ptr_code.end()));
            curr_code.insert(curr_code.end(), std::move_iterator(int_code.begin()), std::move_iterator(int_code.end()));
            
            // 1. 首先将整数乘以指针步长
            Operand scaled_int = Operand::allocOperand(Operand::i32);
            BinaryOpInst mul_inst;
            mul_inst.op = BinaryOpInst::mul;
            mul_inst.op_type = BinaryOpInst::i32;
            mul_inst.dst = scaled_int;
            mul_inst.src1 = int_op;
            mul_inst.src2 = Operand::allocIMM(Operand::i32, 0, TypingNode.helpNum1); // 步长立即数
            curr_code.push_back(mul_inst);
            
            // 2. 将结果转换为指针类型
            Operand int_as_ptr = scaled_int;
            if(scaled_int.datatype != Operand::ptr) {
                AST::CAST_OP cast_op = AST::CAST_OP::INT_TO_PTR;
                int_as_ptr = genCastInst(curr_code, cast_op, scaled_int);
            }
            
            // 3. 执行指针加法
            Operand result_ptr = Operand::allocOperand(Operand::ptr);
            BinaryOpInst add_inst;
            add_inst.op = BinaryOpInst::add;
            add_inst.op_type = BinaryOpInst::ptr; // 指针加法使用整数运算
            add_inst.dst = result_ptr;
            add_inst.src1 = ptr_op;
            add_inst.src2 = int_as_ptr;
            curr_code.push_back(add_inst);
            
            // 存储结果
            IRGenerator->ContextMap[Expr_ptr].code = std::move(curr_code);
            IRGenerator->ContextMap[Expr_ptr].expr_addr = result_ptr;

        } else {
            std::cerr << "Unknown ArithOp\n";
        }
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
        auto derefExpr = static_cast<AST::DerefExpr *>(Expr_ptr);
        auto & TypingNode = IRGenerator->ExprTypeMap->at(derefExpr);
        auto & sub_ctx = Context.at(derefExpr->subExpr.get());
        std::deque<IRinst> curr_code;
        if(TypingNode.ret_is_left_value || TypingNode.retType.basicType == AST::baseType::ARRAY) {
            //作为左值，返回ptr
            Context[Expr_ptr].code = std::move(sub_ctx.code);
            Context[Expr_ptr].expr_addr = std::move(sub_ctx.expr_addr);
            Context.erase(derefExpr->subExpr.get());
        } else {
            //作为右值，则需要取值
            auto Addr = sub_ctx.expr_addr;
            Operand ValOperand = Operand::allocOperand(TypingSystem2OperandType(TypingNode.retType));
            genLoadInst(curr_code,Addr,ValOperand);
            auto addrOperand = genCastInst(curr_code,TypingNode.cast_op,ValOperand);
            Context[Expr_ptr].code = std::move(sub_ctx.code);
            Context[Expr_ptr].code.insert(Context.at(Expr_ptr).code.end(),curr_code.begin(),curr_code.end());
            Context[Expr_ptr].expr_addr = std::move(addrOperand);
            Context.erase(derefExpr->subExpr.get());
        }
    }
    return true;
}

bool parseControlIRGenENTER(AST::Stmt* stmt_ptr, IRGenVisitor* IRGenerator) {
    //在进入时，向布尔表达式分发标签
    auto& Context = IRGenerator->ContextMap;
    if(stmt_ptr->subType == AST::ASTSubType::Branch) {
        auto br_ptr = static_cast<AST::Branch *>(stmt_ptr);
        auto ThenLabel = Operand::allocLabel(u8"THEN");
        auto ELSELabel = Operand::allocLabel(u8"ELSE");
        Context[stmt_ptr].BoolTrueLabel = ThenLabel;
        Context[stmt_ptr].BoolFalseLabel = ELSELabel;
        Context[br_ptr->bool_ptr.get()].BoolTrueLabel = ThenLabel;
        Context[br_ptr->bool_ptr.get()].BoolFalseLabel = ELSELabel;
    }
    if(stmt_ptr->subType == AST::ASTSubType::Loop) {
        auto loop_ptr = static_cast<AST::Loop *>(stmt_ptr);
        auto ThenLabel = Operand::allocLabel(u8"THEN");
        auto ENDLabel = Operand::allocLabel(u8"END");
        Context[loop_ptr].BoolTrueLabel = ThenLabel;
        Context[loop_ptr].BoolFalseLabel = ENDLabel;
        Context[loop_ptr->bool_ptr.get()].BoolTrueLabel = ThenLabel;
        Context[loop_ptr->bool_ptr.get()].BoolFalseLabel = ENDLabel;
    }
    return true;
}

bool parseControlIRGenQUIT(AST::Stmt* stmt_ptr, IRGenVisitor* IRGenerator) {
    auto& Context = IRGenerator->ContextMap;
    
    if(stmt_ptr->subType == AST::ASTSubType::Branch) {
        auto br_ptr = static_cast<AST::Branch*>(stmt_ptr);
        std::deque<IRinst> merged_code;
        
        // 合并条件表达式代码
        merged_code.insert(merged_code.end(), 
                          Context[br_ptr->bool_ptr.get()].code.begin(),
                          Context[br_ptr->bool_ptr.get()].code.end());
        
        if(br_ptr->op == AST::ifOnly) {
            // 生成THEN块代码
            merged_code.push_back(labelInst(Context[stmt_ptr].BoolTrueLabel));
            auto& then_ctx = Context[br_ptr->if_stmt_ptr.get()];
            merged_code.insert(merged_code.end(), then_ctx.code.begin(), then_ctx.code.end());
            merged_code.push_back(labelInst(Context[stmt_ptr].BoolFalseLabel));
            Context.erase(br_ptr->if_stmt_ptr.get());
        } 
        else { // if-else
            // 生成THEN块代码
            merged_code.push_back(labelInst(Context[stmt_ptr].BoolTrueLabel));
            auto& then_ctx = Context[br_ptr->if_stmt_ptr.get()];
            merged_code.insert(merged_code.end(), then_ctx.code.begin(), then_ctx.code.end());
            // 添加跳转到END的指令
            auto ENDLabel = Operand::allocLabel(u8"END");
            BrInst gotoinst;
            gotoinst.is_conditional = false;
            gotoinst.trueLabel = ENDLabel;
            merged_code.push_back(gotoinst);
            // 添加ELSE标签和代码
            merged_code.push_back(labelInst(Context[stmt_ptr].BoolFalseLabel));
            auto& else_ctx = Context[br_ptr->else_stmt_ptr.value().get()];
            merged_code.insert(merged_code.end(), else_ctx.code.begin(), else_ctx.code.end());
            // 添加END标签
            merged_code.push_back(labelInst(ENDLabel));
            Context.erase(br_ptr->if_stmt_ptr.get());
            Context.erase(br_ptr->else_stmt_ptr.value().get());
        }
        
        Context[stmt_ptr].code = std::move(merged_code);
    }
    else if(stmt_ptr->subType == AST::ASTSubType::Loop) {
        auto loop_ptr = static_cast<AST::Loop*>(stmt_ptr);
        std::deque<IRinst> loop_code;
        
        // 生成循环头标签
        auto HEADERLabel = Operand::allocLabel(u8"HEADER");
        loop_code.push_back(labelInst(HEADERLabel));
        
        // 合并条件表达式代码
        loop_code.insert(loop_code.end(),
                        Context[loop_ptr->bool_ptr.get()].code.begin(),
                        Context[loop_ptr->bool_ptr.get()].code.end());
        Context.erase(loop_ptr->bool_ptr.get());
        // 生成THEN块代码
        loop_code.push_back(labelInst(Context[stmt_ptr].BoolTrueLabel));
        auto& body_ctx = Context[loop_ptr->stmt_ptr.get()];
        loop_code.insert(loop_code.end(), body_ctx.code.begin(), body_ctx.code.end());
        Context.erase(loop_ptr->stmt_ptr.get());
        BrInst gotoinst;
        gotoinst.is_conditional = false;
        gotoinst.trueLabel = HEADERLabel;
        loop_code.push_back(gotoinst);
        // 添加END标签
        loop_code.push_back(labelInst(Context[stmt_ptr].BoolFalseLabel));
        
        Context[stmt_ptr].code = std::move(loop_code);
    }
    
    return true;
}

bool parseBoolIRGenENTER(AST::ASTBool* Bool_ptr, IRGenVisitor* IRGenerator) {
    auto& Context = IRGenerator->ContextMap;
    if(Bool_ptr->isBoolOperation) {
        //只有and or需要进行标签分发
        auto currTHEN = Context[Bool_ptr].BoolTrueLabel;
        auto currELSE = Context[Bool_ptr].BoolFalseLabel;
        auto RIGHTLabel = Operand::allocLabel(u8"RIGH");
        Context[Bool_ptr].BoolTmpLabel1 = RIGHTLabel;
        if(Bool_ptr->BoolOP == AST::BOOLAND) {
            Context[Bool_ptr->LBool.get()].BoolTrueLabel = RIGHTLabel;
            Context[Bool_ptr->LBool.get()].BoolFalseLabel = currELSE;   //短路

            Context[Bool_ptr->RBool.get()].BoolTrueLabel = currTHEN;
            Context[Bool_ptr->RBool.get()].BoolFalseLabel = currELSE;
            
        } else if(Bool_ptr->BoolOP == AST::BOOLOR) {
            Context[Bool_ptr->LBool.get()].BoolTrueLabel = currTHEN;    //短路
            Context[Bool_ptr->LBool.get()].BoolFalseLabel = RIGHTLabel;   

            Context[Bool_ptr->RBool.get()].BoolTrueLabel = currTHEN;
            Context[Bool_ptr->RBool.get()].BoolFalseLabel = currELSE;
        } else {
            std::cerr<<"BOOL错误\n";
        }
    }
    return true;
}


bool parseBoolIRGenQUIT(AST::ASTBool* Bool_ptr, IRGenVisitor* IRGenerator) {
    auto& Context = IRGenerator->ContextMap;
    if(Bool_ptr->isBoolOperation) 
    {
        std::deque<IRinst> curr_code;
        Operand bool_val;
        // 左操作数
        auto& left_ctx = Context[Bool_ptr->LBool.get()];
        curr_code.insert(curr_code.end(), left_ctx.code.begin(), left_ctx.code.end());
        
        // 短路标签
        if(!Context[Bool_ptr].BoolTmpLabel1.empty()) {
            curr_code.push_back(labelInst(Context[Bool_ptr].BoolTmpLabel1));
        } else {
            std::cerr<<"内部错误：Bool短路标签缺失\n";
        }
        // 右操作数
        auto& right_ctx = Context[Bool_ptr->RBool.get()];
        curr_code.insert(curr_code.end(), right_ctx.code.begin(), right_ctx.code.end());

        Context[Bool_ptr].code = std::move(curr_code);
        Context[Bool_ptr].expr_addr = bool_val;
    }
    else {
        auto& left_ctx = Context[Bool_ptr->Lval_ptr.get()];
        std::deque<IRinst> curr_code(left_ctx.code.begin(), left_ctx.code.end());
        Operand bool_val = left_ctx.expr_addr; // 默认为单表达式布尔值
        if (Bool_ptr->rop != AST::ROP_ENUM::NoneROP) {
            auto& right_ctx = Context[Bool_ptr->Rval_ptr.get()];
            curr_code.insert(curr_code.end(), right_ctx.code.begin(), right_ctx.code.end());
            BinaryOpInst cmp;
            cmp.op = BinaryOpInst::cmp;
            cmp.src1 = left_ctx.expr_addr;
            cmp.src2 = right_ctx.expr_addr;
            switch(Bool_ptr->rop) {
                case AST::ROP_ENUM::L:  cmp.cmp_type = BinaryOpInst::L;  break;
                case AST::ROP_ENUM::LE: cmp.cmp_type = BinaryOpInst::LE; break;
                case AST::ROP_ENUM::G:  cmp.cmp_type = BinaryOpInst::G;  break;
                case AST::ROP_ENUM::EQ:  cmp.cmp_type = BinaryOpInst::EQ;  break;
                default: return false;
            }
            bool_val = Operand::allocOperand(Operand::i32, u8"cond");
            cmp.dst = bool_val;
            curr_code.push_back(cmp);
        }
        if (!Context[Bool_ptr].BoolTrueLabel.empty() && 
            !Context[Bool_ptr].BoolFalseLabel.empty()) {
            BrInst branch;
            branch.is_conditional = true;
            branch.condition = bool_val;
            branch.trueLabel = Context[Bool_ptr].BoolTrueLabel;
            branch.falseLabel = Context[Bool_ptr].BoolFalseLabel;
            curr_code.push_back(branch);
        }
        else {
            std::cerr<<"Bool没有可用的label\n";
            return false;
        }
        // 4. 保存当前节点上下文
        Context[Bool_ptr].code = std::move(curr_code);
        Context[Bool_ptr].expr_addr = bool_val; // 传递布尔值结果
    }
    
    return true;
}


bool parseCopyNode(AST::ASTNode * node_ptr,IRGenVisitor * IRGenerator) {
    auto & Context = IRGenerator->ContextMap;

    if(node_ptr->subType == AST::ASTSubType::Block) {
        auto * block_ptr = static_cast<AST::Block *>(node_ptr);
        Context[node_ptr].code = std::move(Context[block_ptr->ItemList_ptr.get()].code);
        Context.erase(block_ptr->ItemList_ptr.get());
    }
    if(node_ptr->subType == AST::ASTSubType::BlockStmt) {
        auto * blockS_ptr = static_cast<AST::BlockStmt *>(node_ptr);
        Context[node_ptr].code = std::move(Context[blockS_ptr->block_ptr.get()].code);
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
        Context[node_ptr].code = std::move(code);

    }
    return true;
}


}  //namespace IR