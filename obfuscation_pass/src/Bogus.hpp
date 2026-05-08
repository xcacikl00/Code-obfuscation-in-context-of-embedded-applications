/*
 * @filename: Bogus.hpp
 * @author:   Lukáš Čačík
 * @date:     May 09, 26
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "llvm/IR/PassManager.h"

struct Bogus : public llvm::PassInfoMixin<Bogus> {
    unsigned long long Seed;
    
    // Simple constructor
    explicit Bogus(unsigned long long seed) : Seed(seed) {}

    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
    static bool isRequired() { return true; }
};