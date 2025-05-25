#include "irGen.h"

namespace IR {

u8string IRformat(const IRinst inst) {
    return std::visit(overload {
        // 1. 二元运算指令 (add/mul/cmp/cast)
        [](const BinaryOpInst& op) -> u8string {
            u8string ret;
            ret += op.dst.format() + u8" = ";
            switch (op.op) {
            case BinaryOpInst::MainOP::add:
                ret += u8"add." + BinaryOpInstOPtypeToString(op.op_type) + u8" ";
                break;
            case BinaryOpInst::MainOP::mul:
                ret += u8"mul." + BinaryOpInstOPtypeToString(op.op_type) + u8" ";
                break;
            case BinaryOpInst::MainOP::cmp: {
                u8string cmp_op;
                switch (op.cmp_type) {
                case BinaryOpInst::cmpType::EQ: cmp_op = u8"eq "; break;
                case BinaryOpInst::cmpType::NE: cmp_op = u8"ne "; break;
                case BinaryOpInst::cmpType::GE: cmp_op = u8"ge "; break;
                case BinaryOpInst::cmpType::G:  cmp_op = u8"gt "; break;
                case BinaryOpInst::cmpType::LE: cmp_op = u8"le "; break;
                case BinaryOpInst::cmpType::L:  cmp_op = u8"lt "; break;
                default: cmp_op += u8""; break;
                }
                ret += u8"cmp." + cmp_op;
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
                ret += u8"cast." + cast_op;
                break;
            }
            case BinaryOpInst::MainOP::COPY:
                ret += u8"copy ";
                break;
            default:
                return u8"";
            }
            ret += u8" " + op.src1.format();
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
                return mem.dst.format() + u8" = load " + IRformat(mem.datatype) + u8" " + mem.src.format() + 
                       u8", align " + toU8str(std::to_string(mem.align));
            } else if (mem.memOP == memOpInst::OP::store) {
                return u8"store " + IRformat(mem.datatype) + u8" " + mem.src.format() + u8", " + 
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
                return u8"br " + br.trueLabel.format();
            }
            return u8"br " + br.condition.format() + u8", " + 
                   br.trueLabel.format() + u8", " + br.falseLabel.format();
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

//获取idNode对应的Operand
Operand fetchIdAddr(AST::SymIdNode * id_ptr , IRGenVisitor * IRGenerator) {
    auto SePtr = static_cast<Semantic::SymbolEntry *>(id_ptr->symEntryPtr);
    auto ret = IRGenerator->EntrySymMap->at(SePtr);
    return ret;
}
//idNode对应的数据类型
Datatype fetchIdType(AST::SymIdNode * id_ptr , IRGenVisitor * IRGenerator) {
    auto SePtr = static_cast<Semantic::SymbolEntry *>(id_ptr->symEntryPtr);
    return TypingSystem2IRType(SePtr->Type);
}
//生成cast指令，添加在insts之后，返回cast后的Operand，src不应该是立即数
Operand genCastInst(std::deque<IRinst> & insts ,AST::CAST_OP cast_op , Operand src) {
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
void genLoadInst(std::deque<IRinst> & insts ,Operand srcAddr,Operand dst) {
    memOpInst load_inst;
    load_inst.memOP = memOpInst::load;
    load_inst.src = srcAddr;
    load_inst.dst = dst;
    switch(dst.datatype)
    {
    case Operand::OperandType::f32 :
        load_inst.datatype = f32;
        load_inst.align = 4;
        break;
    case Operand::OperandType::i32 :
        load_inst.datatype = i32;
        load_inst.align = 4;
        break;
    case Operand::OperandType::ptr :
        load_inst.datatype = ptr;
        load_inst.align = PTR_LENGTH_IN_BYTES;
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
void genStoreInst(std::deque<IRinst> & insts ,Operand src,Operand dstAddr) {
    memOpInst store_inst;
    store_inst.memOP = memOpInst::store;
    store_inst.src = src;
    store_inst.dst = dstAddr;
    switch(src.datatype)
    {
    case Operand::OperandType::f32 :
        store_inst.datatype = f32;
        store_inst.align = 4;
        break;
    case Operand::OperandType::i32 :
        store_inst.datatype = i32;
        store_inst.align = 4;
        break;
    case Operand::OperandType::ptr :
        store_inst.datatype = ptr;
        store_inst.align = PTR_LENGTH_IN_BYTES;
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
Operand genCallInst(std::deque<IRinst> & insts ,AST::SymIdNode * func_id,IRGenVisitor * irgen,std::vector<Operand> srcAddrVec) {
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