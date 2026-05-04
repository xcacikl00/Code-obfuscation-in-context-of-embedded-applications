#include "llvm/IR/PassManager.h"

struct InstructionSubstitution : public llvm::PassInfoMixin<InstructionSubstitution>
{
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
    static bool isRequired() { return true; }
};
