#include "llvm/IR/PassManager.h"

struct Bogus : public llvm::PassInfoMixin<Bogus> {
    unsigned long long Seed;
    
    // Simple constructor
    explicit Bogus(unsigned long long seed) : Seed(seed) {}

    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
    static bool isRequired() { return true; }
};