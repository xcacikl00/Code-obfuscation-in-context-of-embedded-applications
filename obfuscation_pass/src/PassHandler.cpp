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

    PB.registerOptimizerLastEPCallback( // should be last or phis will get reintroduced into the code
                                        // in between passes and this can cause issues
        [](ModulePassManager &MPM, OptimizationLevel Level)
        {
            FunctionPassManager FPM;

            bool doSubst = getEnvFlag("OBF_SUBST");
            bool doFlatten = getEnvFlag("OBF_FLATTEN");
            bool doBogus = getEnvFlag("OBF_BOGUS");

            if (doSubst)
                llvm::errs() << "REGISTER: SUBST \n";

            FPM.addPass(InstructionSubstitution());

            if (doFlatten)
            {
                llvm::errs() << "REGISTER: FLATTEN \n";

                FPM.addPass(LowerSwitchPass());
                FPM.addPass(RegToMemPass());
                FPM.addPass(Flattening());
            }

            if (doBogus)
            {
                llvm::errs() << "REGISTER: BOGUS \n";

                FPM.addPass(RegToMemPass());
                FPM.addPass(Bogus(42));
            }

            MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
        });

    PB.registerPipelineParsingCallback( // for opt
        [](StringRef Name, FunctionPassManager &FPM,
           ArrayRef<PassBuilder::PipelineElement>)
        {
            if (Name == "flattening")
            {
                FPM.addPass(Flattening());
                return true;
            }
            if (Name == "bogus")
            {
                FPM.addPass(Bogus(42));
                return true;
            }
            if (Name == "subst")
            {
                FPM.addPass(InstructionSubstitution());
                return true;
            }
            return false;
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

    return getTestPassPluginInfo();
}