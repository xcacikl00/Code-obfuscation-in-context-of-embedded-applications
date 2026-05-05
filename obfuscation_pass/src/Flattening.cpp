
#include "Flattening.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Verifier.h>
#include "llvm/Transforms/Utils/Local.h"
#include <random>
#include "llvm/Transforms/Utils/LowerSwitch.h"

using namespace llvm;

static std::mt19937 &getRng()
{
    static std::mt19937 Rng{std::random_device{}()};
    return Rng;
}

static uint32_t randRange(uint32_t Lo, uint32_t Hi)
{
    return std::uniform_int_distribution<uint32_t>{Lo, Hi}(getRng());
}

//
static uint32_t Encode(uint32_t Id, uint32_t X, uint32_t Y)
{
    return (Id ^ X) + Y;
}

// Unconditional transition: store plainId, volatile reload, re-encode, br Dispatch.
template <class IRBTy>
static void EmitTransition(IRBTy &IRB, AllocaInst *SV, BasicBlock *Dispatch,
                           uint32_t PlainId, uint32_t X, uint32_t Y)
{
    IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), PlainId), SV);
    LoadInst *Val = IRB.CreateLoad(IRB.getInt32Ty(), SV, /*isVolatile=*/true);
    IRB.CreateStore(
        IRB.CreateAdd(
            IRB.CreateXor(Val, ConstantInt::get(IRB.getInt32Ty(), X)),
            ConstantInt::get(IRB.getInt32Ty(), Y)),
        SV, /*isVolatile=*/true);
    IRB.CreateBr(Dispatch);
}

// Conditional transition: uses two temp allocas to prevent the optimiser
// folding the select when both encoded IDs are compile-time constants.
template <class IRBTy>
static void EmitTransition(IRBTy &IRB, AllocaInst *SV, BasicBlock *Dispatch,
                           AllocaInst *TmpTrue, AllocaInst *TmpFalse,
                           Value *Cond,
                           uint32_t TruePlainId, uint32_t FalsePlainId,
                           uint32_t X, uint32_t Y)
{
    IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), TruePlainId), TmpTrue);
    IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), FalsePlainId), TmpFalse);
    LoadInst *LTrue = IRB.CreateLoad(IRB.getInt32Ty(), TmpTrue, /*isVolatile=*/true);
    LoadInst *LFalse = IRB.CreateLoad(IRB.getInt32Ty(), TmpFalse, /*isVolatile=*/true);
    Value *EncTrue = IRB.CreateAdd(IRB.CreateXor(LTrue, ConstantInt::get(IRB.getInt32Ty(), X)),
                                   ConstantInt::get(IRB.getInt32Ty(), Y));
    Value *EncFalse = IRB.CreateAdd(IRB.CreateXor(LFalse, ConstantInt::get(IRB.getInt32Ty(), X)),
                                    ConstantInt::get(IRB.getInt32Ty(), Y));
    IRB.CreateStore(IRB.CreateSelect(Cond, EncTrue, EncFalse), SV, /*isVolatile=*/true);
    IRB.CreateBr(Dispatch);
}

static uint32_t getSwitchEnc(const DenseMap<BasicBlock *, uint32_t> &SwitchEnc,
                             BasicBlock *BB)
{
    auto It = SwitchEnc.find(BB);
    if (It == SwitchEnc.end())
        llvm_unreachable("Flattening: unmapped successor block");
    return It->second;
}

static bool flattenFunction(Function &F, uint32_t X, uint32_t Y)
{
    if (F.getInstructionCount() == 0)
        return false;


    // do not flatten anything containig error handling, the logic to handle
    // unwinding as exptected by the compiler is not implemented
    for (BasicBlock &BB : F)
    {
        if (BB.isLandingPad() || BB.isEHPad())
            return false;
        if (isa<InvokeInst>(BB.getTerminator()))
            return false;
    }

    for (BasicBlock &BB : F)
    {
        SmallVector<PHINode *, 4> PHIs;
        for (PHINode &Phi : BB.phis())
            PHIs.push_back(&Phi);
        for (PHINode *Phi : PHIs)
            DemotePHIToStack(Phi);
    }

    // make sure that lowerSwitch executed
    for (BasicBlock &BB : F)
    {
        if (auto *SI = dyn_cast<SwitchInst>(BB.getTerminator()))
        {
        // found a switch instruction these should never be here
        llvm::errs() << "@@@@@@@@@ FOUND SWITCH INSTRUCTION @@@@@@@@@";
            throw 1;
        }
    }

    // running the lower switch pass causes dominance issues,
    // demote any such cases
    for (BasicBlock &BB : F)
    {
        SmallVector<Instruction *, 16> ToDemote;
        for (Instruction &I : BB)
        {
            // Skip Phis (already handled) and allocas
            if (isa<PHINode>(I) || isa<AllocaInst>(I) || I.isTerminator())
                continue;

            for (User *U : I.users())
            {
                if (auto *UI = dyn_cast<Instruction>(U))
                {
                    if (UI->getParent() != &BB)
                    {
                        ToDemote.push_back(&I);
                        break;
                    }
                }
            }
        }
        for (Instruction *I : ToDemote)
        {
            DemoteRegToStack(*I);
        }
    }

    BasicBlock *EntryBlock = &F.getEntryBlock();
    SmallVector<BasicBlock *, 20> FlattedBBs;
    for (BasicBlock &BB : F)
        if (&BB != EntryBlock)
            FlattedBBs.push_back(&BB);

    if (FlattedBBs.size() <= 1)
        return false;

    // target block for the dispatcher to execute first
    BasicBlock *InitialTarget = nullptr;

    // defensively demote all phis, the reg2mem pass should catch them
    // however when compiling cmsis DSP there were malformed phis after the pass which
    // caused the compilation to file even when reg2meme was  ran beforehand

    if (auto *Br = dyn_cast<BranchInst>(EntryBlock->getTerminator()))
    {
        if (Br->isConditional())
        {
            BasicBlock *EntrySplit = EntryBlock->splitBasicBlockBefore(Br, "EntrySplit");
            FlattedBBs.insert(FlattedBBs.begin(), EntryBlock);
            EntryBlock = EntrySplit;
            InitialTarget = FlattedBBs[0];
        }
        else
        {
            // capture successor before deleting branch
            InitialTarget = Br->getSuccessor(0);
        }
    }

    EntryBlock->getTerminator()->eraseFromParent();

    // assing each block a unique random ID
    DenseMap<BasicBlock *, uint32_t> SwitchEnc;
    SmallSet<uint32_t, 30> UsedRnd;
    for (BasicBlock *BB : FlattedBBs)
    {
        uint32_t PlainId;
        do
        {
            PlainId = randRange(10, UINT32_MAX - 1);
            uint32_t EncId = Encode(PlainId, X, Y);
            if (!UsedRnd.count(PlainId) && !UsedRnd.count(EncId))
            {
                UsedRnd.insert(PlainId);
                UsedRnd.insert(EncId);
                break;
            }
        } while (true);
        SwitchEnc[BB] = PlainId;
    }

    // Build dispatch infrastructure.
    LLVMContext &Ctx = F.getContext();
    auto *FlatLoopEntry = BasicBlock::Create(Ctx, "FlatLoopEntry", &F, EntryBlock);
    auto *FlatLoopEnd = BasicBlock::Create(Ctx, "FlatLoopEnd", &F, EntryBlock);
    auto *DefaultCase = BasicBlock::Create(Ctx, "DefaultCase", &F, FlatLoopEnd);

    IRBuilder<> EntryIR(EntryBlock);
    AllocaInst *SwitchVar = EntryIR.CreateAlloca(EntryIR.getInt32Ty(), 0, "SwitchVar");
    AllocaInst *TmpTrue = EntryIR.CreateAlloca(EntryIR.getInt32Ty(), 0, "TmpTrue");
    AllocaInst *TmpFalse = EntryIR.CreateAlloca(EntryIR.getInt32Ty(), 0, "TmpFalse");
    EntryIR.CreateStore(
        EntryIR.getInt32(Encode(getSwitchEnc(SwitchEnc, InitialTarget), X, Y)),
        SwitchVar);
    EntryIR.CreateBr(FlatLoopEntry);

    EntryBlock->moveBefore(FlatLoopEntry);
    IRBuilder<>(FlatLoopEnd).CreateBr(FlatLoopEntry);
    IRBuilder<>(DefaultCase).CreateUnreachable();

    IRBuilder<> DispIR(FlatLoopEntry);
    LoadInst *LoadSV = DispIR.CreateLoad(DispIR.getInt32Ty(), SwitchVar,
                                         /*isVolatile=*/true, "SwitchVar");
    SwitchInst *Switch = DispIR.CreateSwitch(
        LoadSV, DefaultCase, static_cast<unsigned>(FlattedBBs.size()));

    for (BasicBlock *BB : FlattedBBs)
    {
        uint32_t EncId = Encode(getSwitchEnc(SwitchEnc, BB), X, Y);
        BB->moveBefore(FlatLoopEnd);
        Switch->addCase(
            cast<ConstantInt>(ConstantInt::get(Switch->getCondition()->getType(), EncId)),
            BB);
    }

    // rewrite terminators.
    for (BasicBlock *BB : FlattedBBs)
    {
        Instruction *Term = BB->getTerminator();

        if (isa<ReturnInst>(Term) || isa<UnreachableInst>(Term))
            continue;

        auto *Br = dyn_cast<BranchInst>(Term);
        if (!Br)
            llvm_unreachable("Flattening: unexpected non-branch terminator");

        if (Br->isUnconditional())
        {
            uint32_t PlainId = getSwitchEnc(SwitchEnc, Br->getSuccessor(0));
            IRBuilder<> IRB(Br);
            EmitTransition(IRB, SwitchVar, FlatLoopEnd, PlainId, X, Y);
            Br->eraseFromParent();
            continue;
        }

        uint32_t TruePlain = getSwitchEnc(SwitchEnc, Br->getSuccessor(0));
        uint32_t FalsePlain = getSwitchEnc(SwitchEnc, Br->getSuccessor(1));
        IRBuilder<> IRB(Br);
        EmitTransition(IRB, SwitchVar, FlatLoopEnd,
                       TmpTrue, TmpFalse, Br->getCondition(),
                       TruePlain, FalsePlain, X, Y);
        Br->eraseFromParent();
    }

    return true;
}

PreservedAnalyses Flattening::run(Function &F, FunctionAnalysisManager &AM)
{
    if (F.isDeclaration() || F.isIntrinsic())
        return PreservedAnalyses::all();

    const uint32_t X = randRange(10, 254);
    const uint32_t Y = randRange(10, 254);
    //  llvm::errs() << "flattening is running  on: " << F.getName() << "\n";

    LowerSwitchPass LSP;
    LSP.run(F, AM);

    bool Changed = flattenFunction(F, X, Y);
    Changed = true;
    if (verifyFunction(F, &errs()))
    {
        report_fatal_error("Obfuscation pass produced invalid IR!");
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}