#ifndef LCMP_ASM_GEN_HEADER
#define LCMP_ASM_GEN_HEADER
#include "irGen.h"


namespace ASM {

using std::string;
using std::u8string;



class X64ASMGenerator
{
public:
    bool genASM(IR::IRContent & IRs,std::ostream & out = std::cout);
};




void test_X64ASM_main();






} // namespace ASM



#endif



// std::visit(overload {
// [&](IR::BinaryOpInst& op) {
//     if(op.op == IR::BinaryOpInst::add) {
//     return true;
// },
// [&](IR::callOpInst& op) {
//     
//     return true;
// },
// [&](IR::allocaOpInst& op) {
//     return true;
// },
// [&](IR::memOpInst& op) {
//     return true;
// },
// [&](IR::retInst& op) {
//     return true;
// },
// [&](IR::printInst& op) {
//     return true;
// },
// [&](IR::BrInst& op) {
//     return true;
// },
// [&](IR::phiInst& op) {
//     return false;
// },
// [&](IR::labelInst& op) {
//     return true;            
// }
// }, inst);