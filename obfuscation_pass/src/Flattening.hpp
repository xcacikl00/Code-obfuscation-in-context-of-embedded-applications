#include "llvm/IR/PassManager.h"

struct Flattening : public llvm::PassInfoMixin<Flattening>
{
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
    static bool isRequired() { return true; }
};
