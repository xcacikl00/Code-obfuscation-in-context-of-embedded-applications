#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/LowerSwitch.h"
#include "llvm/Transforms/Scalar/Reg2Mem.h"
#include "InstructionSubstitution.hpp"
#include "Flattening.hpp"
#include "Bogus.hpp"
#include <cstdlib> // std::getenv

using namespace llvm;

// helper: returns true if env var is set to '1'
static bool getEnvFlag(const char *Name)
{
    const char *Val = std::getenv(Name);
    return Val && Val[0] == '1';
}

void PassBuilderHook(PassBuilder &PB)
{

    PB.registerVectorizerStartEPCallback(
        [](FunctionPassManager &FPM, OptimizationLevel Level)
        {
            llvm::errs() << "DEBUG: PassBuilderHook called\n";

            // Check environment variables instead of CLI opts

            bool doSubst = getEnvFlag("OBF_SUBST");
            bool doFlatten = getEnvFlag("OBF_FLATTEN");
            bool doBogus = getEnvFlag("OBF_BOGUS");

            if (doSubst)
            {
                FPM.addPass(InstructionSubstitution());
            }

            if (doFlatten)
            {
                // lower switches to ifs to simplify flattening logic
                FPM.addPass(LowerSwitchPass());
                // demote registers to memory to remove PHI nodes
                FPM.addPass(RegToMemPass());
                FPM.addPass(Flattening());
            }

            if (doBogus)
            {
                // remove PHIs for block shuffling
                FPM.addPass(RegToMemPass());
                FPM.addPass(Bogus(42));
            }
        });
}

PassPluginLibraryInfo getTestPassPluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION,
        "ObfuscatorPass",
        LLVM_VERSION_STRING,
        PassBuilderHook};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    llvm::errs() << "DEBUG: llvmGetPassPluginInfo() called\n";

    return getTestPassPluginInfo();
}