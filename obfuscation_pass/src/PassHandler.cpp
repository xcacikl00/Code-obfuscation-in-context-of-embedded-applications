#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/LowerSwitch.h"
#include "llvm/Transforms/Scalar/Reg2Mem.h"
#include "InstructionSubstitution.hpp"
#include "Flattening.hpp"
#include "Bogus.hpp"
#include <cstdlib> // std::getenv
#include "stdint.h"

using namespace llvm;

// helper: returns true if env var is set to '1'
static bool getEnvFlag(const char *Name)
{
    const char *Val = std::getenv(Name);
    return Val && Val[0] == '1';
}

void PassBuilderHook(PassBuilder &PB)
{

    PB.registerScalarOptimizerLateEPCallback(
        [](FunctionPassManager &FPM, OptimizationLevel Level)
        {
            bool doSubst = getEnvFlag("OBF_SUBST");
            bool doFlatten = getEnvFlag("OBF_FLATTEN");
            bool doBogus = getEnvFlag("OBF_BOGUS");
            bool debug_print = getEnvFlag("OBF_DEBUG");
            char *seed_string = std::getenv("OBF_SEED");
            unsigned long long seed = 0xABCDBCDAABCDBCDA; // default
            char *endPtr;
            if (seed_string)
            {
                seed = std::strtoull(seed_string,&endPtr,16);
                
            }

            if (doSubst)
            {
                if (debug_print)
                {
                    llvm::errs() << "REGISTER: SUBSTITUTION \n";
                }
                FPM.addPass(InstructionSubstitution());
            }

            if (doFlatten)
            {
                if (debug_print)
                {
                    llvm::errs() << "REGISTER: FLATTENING \n";
                }
                FPM.addPass(RegToMemPass()); // reg2mem pass could probably be removed
                FPM.addPass(Flattening());
            }

            if (doBogus)
            {
                if (debug_print)
                {
                    
                    llvm::errs() << "REGISTER: BOGUS \n";
                    llvm::errs() << "USING SEED: "<< format_hex(seed, 16, true) <<"\n";
                }

                FPM.addPass(RegToMemPass());
                FPM.addPass(Bogus(seed));
            }
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
                FPM.addPass(RegToMemPass());
                FPM.addPass(Bogus(42));
                return true;
            }
            if (Name == "subst")
            {
                FPM.addPass(RegToMemPass());
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