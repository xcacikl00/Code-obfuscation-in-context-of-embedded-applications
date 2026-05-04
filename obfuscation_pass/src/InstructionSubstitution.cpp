#include "InstructionSubstitution.hpp"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/Casting.h"
using namespace llvm;

// Internal helper

namespace
{ // no need to pollute global namespace with internal functions

    void SubstituteAdd(Instruction &I)

    {

        if (!I.getType()->isIntegerTy())
        {
            return;
        }

        //  a + b = (a XOR b) + (2 * (a AND b))
        IRBuilder<> Builder(&I);

        Value *a = I.getOperand(0);
        Value *b = I.getOperand(1);

        //  (a ^ b)
        Value *xorVal = Builder.CreateXor(a, b, "subst_xor");

        //  (a & b)
        Value *andVal = Builder.CreateAnd(a, b, "subst_and");

        //  2 * (a & b)
        Value *twoAnd = Builder.CreateShl(andVal, ConstantInt::get(andVal->getType(), 1), "subst_shl"); // shl requires value and operand to have the same type
        // (a ^ b) + (2 * (a & b))
        Value *finalAdd = Builder.CreateAdd(xorVal, twoAnd, "subst_final");

        I.replaceAllUsesWith(finalAdd);
        I.eraseFromParent();
    }

    void SubstituteSub(Instruction &I)

    {

        // llvm allows sub to be run on vectors which would cause an error because of getType(), only substitute
        if (!I.getType()->isIntegerTy())
        {
            return;
        }

        // a - b  = (a xor b) - ( (not a and b) shl 1 )
        IRBuilder<> Builder(&I);
        Value *a = I.getOperand(0);
        Value *b = I.getOperand(1);
        // (a xor b)
        Value *firstOperand = Builder.CreateXor(a, b, "subst_xor");

        // (not a)
        Value *notA = Builder.CreateNot(a, "notA");
        // ( not a and b )
        Value *andVal = Builder.CreateAnd(notA, b, "substAnd");
        // ( (not a and b) shl 1 )
        Value *secondOperand = Builder.CreateShl(andVal, ConstantInt::get(andVal->getType(), 1), "subst_RHS");
        // a - b  ->  (a xor b) - ( (not a and b) shl 1 )
        Value *finalSub = Builder.CreateSub(firstOperand, secondOperand, "subst_sub");

        I.replaceAllUsesWith(finalSub);
        I.eraseFromParent();
    }

    void SubstituteOr(Instruction &I)

    {

        if (!I.getType()->isIntegerTy())
        {
            return;
        }

        IRBuilder<> Builder(&I);

        Value *a = I.getOperand(0);
        Value *b = I.getOperand(1);
        // a | b =  (a + b) + 1 + (not a or not b)

        Value *notA = Builder.CreateNot(a, "subst_not_a");
        Value *notB = Builder.CreateNot(b, "subst_not_b");
        // (not a or not b)
        Value *orValue = Builder.CreateOr(notA, notB, "subst_or");

        Value *aPlusB = Builder.CreateAdd(a, b, "subst_a_plus_b");
        // (a + b) + 1
        Value *plusOne = Builder.CreateAdd(aPlusB, ConstantInt::get(aPlusB->getType(), 1), "subst_plus_one");

        // (a + b) + 1 + (not a or not b)
        Value *finalValue = Builder.CreateAdd(plusOne, orValue, "subst_or_final");

        I.replaceAllUsesWith(finalValue);
        I.eraseFromParent();
    }

    void SubstituteXor(Instruction &I)
    {
        if (!I.getType()->isIntegerTy())
        {
            return;
        }
        // a xor b = (a or b) - (a and b)
        IRBuilder<> Builder(&I);
        Value *a = I.getOperand(0);
        Value *b = I.getOperand(1);

        // (a or b)
        Value *orVal = Builder.CreateOr(a, b, "subst_xor_or");

        // (a and b)
        Value *andVal = Builder.CreateAnd(a, b, "subst_xor_and");

        // (a or b) - (a and b)
        Value *finalXor = Builder.CreateSub(orVal, andVal, "subst_xor_final");

        I.replaceAllUsesWith(finalXor);
        I.eraseFromParent();
    }

    void SubstituteAnd(Instruction &I)
    {
        if (!I.getType()->isIntegerTy())
        {
            return;
        }
        // a & b = (a + b) - (a or b)
        IRBuilder<> Builder(&I);
        Value *a = I.getOperand(0);
        Value *b = I.getOperand(1);

        // (a + b)
        Value *sum = Builder.CreateAdd(a, b, "subst_and_sum");

        // (a or b)
        Value *orVal = Builder.CreateOr(a, b, "subst_and_or");

        // (a + b) - (a or b)
        Value *finalAnd = Builder.CreateSub(sum, orVal, "subst_and_final");

        I.replaceAllUsesWith(finalAnd);
        I.eraseFromParent();
    }

}

PreservedAnalyses InstructionSubstitution::run(Function &F, FunctionAnalysisManager &AM)
{
    llvm::errs() << "instruction subst is running  on: " << F.getName() << "\n";
    for (BasicBlock &BB : F)
    {

        for (auto InstrucitonIterator = BB.begin(), IE = BB.end(); InstrucitonIterator != IE;) // need to iterate manually when modifying instructions
        {
            Instruction &I = *InstrucitonIterator++; 


            switch (I.getOpcode())
            {
            case Instruction::Sub:
                SubstituteSub(I);
                break;

            case Instruction::Add:
                SubstituteAdd(I);
                break;

            case Instruction::Or:
                SubstituteOr(I);
                break;

            case Instruction::Xor:
                SubstituteXor(I);
                break;

            case Instruction::And:
                SubstituteAnd(I);
                break;
            }
        }
    }
    return PreservedAnalyses::none();
}