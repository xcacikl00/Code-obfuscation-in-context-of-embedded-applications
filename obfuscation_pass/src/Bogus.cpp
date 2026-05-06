#include "Bogus.hpp"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <random>
#include <llvm/IR/Verifier.h>
#include "llvm/Transforms/Utils/Local.h"
using namespace llvm;

// injects incrementing of global variable to the start of the function to impede dynamic analysis

namespace
{

    Value *injectGlobalUpdate(Function &F)
    {
        Module *M = F.getParent();
        LLVMContext &Ctx = M->getContext();
        Type *Int32Ty = Type::getInt32Ty(Ctx);

        // get or create if doesn't exist
        GlobalVariable *G = M->getNamedGlobal("global_opaque_predicate_var");
        if (!G)
        {
            G = new GlobalVariable(*M, Int32Ty, false,
                                   GlobalValue::InternalLinkage,
                                   ConstantInt::get(Int32Ty, 0),
                                   "global_opaque_predicate_var");
        }

        IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());

        // load, add, store
        LoadInst *OldVal = Builder.CreateLoad(Int32Ty, G, "global_pred_load");
        Value *NewVal = Builder.CreateAdd(OldVal, ConstantInt::get(Int32Ty, 1), "global_pred_inc");
        Builder.CreateStore(NewVal, G);

        return OldVal; // return value to be used in predicate
    }

    BasicBlock *createJunkBlock(BasicBlock *Orig)
    {
        ValueToValueMapTy VMap;
        BasicBlock *JunkBB = CloneBasicBlock(Orig, VMap, ".junk", Orig->getParent());

        // map internal references to the cloned instructions
        for (Instruction &I : *JunkBB)
        {
            RemapInstruction(&I, VMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);
        }

        for (auto it = JunkBB->begin(), e = JunkBB->end(); it != e;) // since I will be delting instructions iterate manually
        // or else the iterator would break
        {
            Instruction &I = *it++;

            if (auto *BinOp = dyn_cast<BinaryOperator>(&I))
            {
                if (BinOp->getOpcode() == Instruction::Add)
                {
                    BinaryOperator *New = BinaryOperator::CreateSub(
                        BinOp->getOperand(0), BinOp->getOperand(1), "", BinOp);
                    BinOp->replaceAllUsesWith(New);
                    BinOp->eraseFromParent();
                }
            }
        }
        return JunkBB;
    }

    Value *createOpaquePredCondition(BasicBlock *BB, Value *X)
    {
        // ((x * x) % 3) != 2
        IRBuilder<> Builder(BB);

        //  ensure we  are working with integers
        Type *ITy = X->getType();
        if (!ITy->isIntegerTy())
        {
            return Builder.getTrue(); // fallback
        }

        // 1. X + 1
        Value *XPlusOne = Builder.CreateAdd(X, ConstantInt::get(ITy, 1), "xplus1");

        // 2. X * (X + 1)
        Value *Mul = Builder.CreateMul(X, XPlusOne, "mul_even");

        // 3. (X * (X + 1)) % 2
        // Using URem (Unsigned) is vital here.
        Value *Two = ConstantInt::get(ITy, 2);
        Value *Rem = Builder.CreateURem(Mul, Two, "rem_two");

        // 4. Condition: Rem == 0 (Always True)
        Value *Zero = ConstantInt::get(ITy, 0);
        return Builder.CreateICmpEQ(Rem, Zero, "opaque_cmp");
    }

    void applyBCF(BasicBlock *BB, Value *Entropy)
    {
        Function *F = BB->getParent();
        Module *M = F->getParent();
        BasicBlock *junkBB = createJunkBlock(BB);

        Instruction *splitPt = &*BB->getFirstInsertionPt();
        BasicBlock *tailBB = BB->splitBasicBlock(splitPt, "real_logic");

        // remove the generated branch from the split
        BB->getTerminator()->eraseFromParent();

        Value *Cond = createOpaquePredCondition(BB, Entropy);

        // branch if true -> real, if false -> junk
        BranchInst::Create(tailBB, junkBB, Cond, BB);

        // redirect junk to real
        junkBB->getTerminator()->eraseFromParent();
        BranchInst::Create(tailBB, junkBB);
    }

}

PreservedAnalyses Bogus::run(Function &F, FunctionAnalysisManager &AM)
{

    if (F.isDeclaration())
        return PreservedAnalyses::all();

    // mix the seed with the function hash to ensure different functions
    // get different obfuscation patterns even with the same global seed.
    std::mt19937 engine(Seed ^ F.getGUID());
    std::bernoulli_distribution dist(0.30); // 30% probability

    Value *predicate_global = nullptr;

    // collect blocks first to avoid iterator invalidation during splitting
    std::vector<BasicBlock *> TargetBlocks;
    for (BasicBlock &BB : F)
    {
        // skip entry block to prevent crashed due to global variable not being loaded yet
        if (&BB == &F.getEntryBlock())
            continue;

        //  skip pads
        if (BB.isLandingPad() || BB.isEHPad())
            continue;

        //  skip blocks with Invoke terminators
        if (isa<InvokeInst>(BB.getTerminator()))
            continue;

        // these are used for exception handling, the logic to properly split these blocks is not implemented, target other blocks instead
        TargetBlocks.push_back(&BB);
    }
    // defensively demote all phis, the reg2mem pass should catch them
    // however when compiling cmsis DSP there were malformed phis after the pass which
    // caused the compilation to file even when reg2meme was  ran beforehand

    for (BasicBlock &BB : F)
    {
        SmallVector<PHINode *, 4> PHIs;
        for (PHINode &Phi : BB.phis())
            PHIs.push_back(&Phi);
        for (PHINode *Phi : PHIs)
            DemotePHIToStack(Phi);
    }

    for (BasicBlock *BB : TargetBlocks)
    {

        if (dist(engine))
        {
            if (!predicate_global)
                predicate_global = injectGlobalUpdate(F);

            applyBCF(BB, predicate_global);
        }
    }

    if (verifyFunction(F, &errs()))
    {
        report_fatal_error("Obfuscation pass produced invalid IR!");
    }

    return PreservedAnalyses::none();
}