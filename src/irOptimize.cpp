#include "irGen.h"

namespace IR {

//寄存器重命名
void renameOperandInPlace(Operand& target, const Operand& from, const Operand& to) {
    if (target == from) {
        target = to;
    }
}
void renameOperand(IRinst& inst, const Operand& from, const Operand& to) {
    std::visit(overload {
        [&from,&to](BinaryOpInst& op) {
            renameOperandInPlace(op.dst, from, to);
            renameOperandInPlace(op.src1, from, to);
            renameOperandInPlace(op.src2, from, to);
        },
        [&from,&to](callOpInst& op) {
            renameOperandInPlace(op.ret, from, to);
            renameOperandInPlace(op.targetFUNC, from, to);
            for (auto& arg : op.srcs) {
                renameOperandInPlace(arg, from, to);
            }
        },
        [&from,&to](allocaOpInst& op) {
            renameOperandInPlace(op.ret, from, to);
        },
        [&from,&to](memOpInst& op) {
            renameOperandInPlace(op.src, from, to);
            renameOperandInPlace(op.dst, from, to);
        },
        [&from,&to](retInst& op) {
            renameOperandInPlace(op.src, from, to);
        },
        [&from,&to](printInst& op) {
            renameOperandInPlace(op.src, from, to);
        },
        [&from,&to](BrInst& op) {
            renameOperandInPlace(op.condition, from, to);
            renameOperandInPlace(op.trueLabel, from, to);
            renameOperandInPlace(op.falseLabel, from, to);
        },
        [&from,&to](phiInst& op) {
            renameOperandInPlace(op.dst, from, to);
            for (auto& [tag, operand] : op.srcs) {
                renameOperandInPlace(tag, from, to);
                renameOperandInPlace(operand, from, to);
            }
        },
        [&from,&to](labelInst& op) {
            renameOperandInPlace(op.label, from, to);
        }
    }, inst);
}

void renameOperandsInBlock(std::vector<IRinst>& block, const Operand& from, const Operand& to) {
    for (auto& inst : block) {
        renameOperand(inst, from, to);
    }
}


bool splitIRblock(FunctionIR & funcIR) {
    //包含无用代码清理（ret之后的）
    funcIR.IRoptimized = std::make_unique<IROptimized>();
    size_t curr_head = 0;
    bool has_clean = false;
    while (curr_head < funcIR.Instblock.size()) {
        auto & curr_inst = funcIR.Instblock[curr_head];
        if(curr_head == 0 ) {
            if(auto label = std::get_if<labelInst>(&curr_inst)) {
                
            }  else {
                std::cerr<<"IR内部错误，起始块没有label\n";
                return 0;
            }  
        }
        if(auto label = std::get_if<labelInst>(&curr_inst)) {
            auto & inst = *label;
            auto baseBlock = std::make_unique<IRBaseBlock>();
            baseBlock->enterLabel = inst.label;
            for(size_t i = curr_head ; i <= funcIR.Instblock.size();i++) {
                if( i == funcIR.Instblock.size()) {
                    std::cerr<<"IR生成失败，控制流不存在ret命令\n";
                    return false;
                }
                //只有branch（含无条件跳转）和ret可以作为基本块终点，label目前可以，不过后面可能可以合并（唯一前继）
                if(auto ret = std::get_if<retInst>(&funcIR.Instblock[i])) {
                    baseBlock->Insts.push_back(funcIR.Instblock[i]);
                    curr_head = i+1;
                    break;
                } else if(auto br = std::get_if<BrInst>(&funcIR.Instblock[i])) {
                    baseBlock->Insts.push_back(funcIR.Instblock[i]);
                    baseBlock->successorLabel.push_back(br->trueLabel);
                    if(br->is_conditional) {
                        baseBlock->successorLabel.push_back(br->falseLabel);
                    }
                    curr_head = i+1;
                    break;
                } else if(curr_head != i) {
                    if(auto lb = std::get_if<labelInst>(&funcIR.Instblock[i])) {
                        curr_head = i;
                        baseBlock->successorLabel.push_back(lb->label);
                        break;
                    }
                }
                baseBlock->Insts.push_back(funcIR.Instblock[i]);
            }
            funcIR.IRoptimized->BlockLists.emplace_back(std::move(baseBlock));
        } else {
            funcIR.Instblock.erase(funcIR.Instblock.begin()+curr_head);
            has_clean = true;
            continue;
        }
    }   
    if(has_clean) {
        std::cerr<<"已清理不可达指令\n";
    }
    //更新前继和后继
    for(auto & baseBlock : funcIR.IRoptimized->BlockLists) {
        for(auto & scBlock : funcIR.IRoptimized->BlockLists) {
            for(auto & tglabel : baseBlock->successorLabel) {
                    if(scBlock->enterLabel == tglabel) {
                    baseBlock->successor.insert(scBlock.get());
                    scBlock->predecessor.insert(baseBlock.get());
                }
            }
        }
    }
    return true;
}

bool mergeLinearBlocks(FunctionIR& funcIR) {
    if (!funcIR.IRoptimized) return false;
    bool changed = false;
    auto& blocks = funcIR.IRoptimized->BlockLists;
    //迭代不动点朴素算法，复杂度稍微有点高
    while (1) {
        bool mergedInThisPass = false;
        for (size_t i = 0; i < blocks.size(); ) {
            auto& currentBlock = blocks[i];
            // 检查是否可以合并当前块和它的唯一后继
            if (currentBlock->successor.size() != 1) {
                i++;
                continue;
            }
            auto successor = *currentBlock->successor.begin();
                
                // 检查后继是否只有当前块作为前驱
            if (successor->predecessor.size() == 1 && 
                *successor->predecessor.begin() == currentBlock.get()) {
                
                auto it = std::find_if(blocks.begin(), blocks.end(), 
                    [successor](const auto& ptr) { return ptr.get() == successor; });
                
                //执行合并逻辑
                if (it != blocks.end()) {
                    bool skipFirst = true;
                    for (auto& inst : (*it)->Insts) {
                        if (skipFirst && std::holds_alternative<labelInst>(inst)) {
                            skipFirst = false;
                            continue;
                        }
                        skipFirst = false;
                        currentBlock->Insts.push_back(inst);
                    }    
                    currentBlock->successor = successor->successor;
                    currentBlock->successorLabel = successor->successorLabel;
                    for (auto newSucc : successor->successor) {
                        newSucc->predecessor.erase(successor);
                        newSucc->predecessor.insert(currentBlock.get());
                    }
                    blocks.erase(it);
                    mergedInThisPass = true;
                    changed = true;
                    continue;
                }
            }
            i++;
        }
        if (!mergedInThisPass) break;
    }
    
    return changed;
}

}   //namespace IR