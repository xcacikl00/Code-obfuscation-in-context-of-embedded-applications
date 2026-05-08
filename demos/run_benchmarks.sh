# @filename: run_benchmarks.sh
# @author:   Lukáš Čačík
# @date:     May 09, 26
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


#!/bin/bash

OBFS=( "OBF_BOGUS" "OBF_SUBST" "OBF_FLATTEN")
LOG_FILE="benchmark_results.log"
export OBF_SEED="ABCDBCDAABCDBCDA"
export LLVM_OBF_SEED="0xA04252B187478C00A40BC6D81D1A8A52"
THESIS_PLUGIN="$(realpath ../obfuscation_pass/build/libObfuscator.so)"
VALIDATION_PLUGIN="$(realpath ../obfuscation_pass/obfuscator-llvm/build/libLLVMObfuscator.so)"

declare -A PASS_MAP
PASS_MAP["OBF_SUBST"]="substitution"
PASS_MAP["OBF_BOGUS"]="bogus"
PASS_MAP["OBF_FLATTEN"]="flattening"

echo "Target Device: STM32F411 (Black Pill)" | tee -a $LOG_FILE

run_bench() {
    local label=$1
    local plugin_path=$2
    echo "Benchmarking with $label" | tee -a $LOG_FILE
    echo "Using Obfuscator Plugin: $(basename "$plugin_path")" | tee -a $LOG_FILE

    mkdir -p build
    cd build
    cmake .. -DPLUGIN_PATH="$plugin_path"
    make clean
    make -j$(nproc)
    cd ..

    bash ./benchmark.sh 
}

export OBF_DEBUG=1
export OBF_SUBST=0
export OBF_BOGUS=0
export OBF_FLATTEN=0
export OBF_EARLY=0
export LLVM_OBF_SCALAROPTIMIZERLATE_PASSES=""
export LLVM_OBF_OPTIMIZERLASTEP_PASSES=""


run_for_plugin() {
CURRENT_PLUGIN=$1
run_bench "NO_OBFS" "$CURRENT_PLUGIN"

# resgister at OptimizerLastEp
echo "=== REGISTERING AT OptimizerLastEp ===" | tee -a $LOG_FILE
export OBF_EARLY=0


for target in "${OBFS[@]}"; do
    export OBF_SUBST=0; export OBF_BOGUS=0; export OBF_FLATTEN=0
    export "$target"=1
    export LLVM_OBF_OPTIMIZERLASTEP_PASSES="${PASS_MAP[$target]}" 
    export LLVM_OBF_SCALAROPTIMIZERLATE_PASSES=""
    run_bench "$target" "$CURRENT_PLUGIN"
done

export OBF_SUBST=1; export OBF_BOGUS=1; export OBF_FLATTEN=1;
export LLVM_OBF_OPTIMIZERLASTEP_PASSES="flattening,bogus,substitution"
run_bench "ALL_OBFS" "$CURRENT_PLUGIN"
echo "\n"

# register at ScalarOptimizerLate
echo "=== REGISTERING AT ScalarOptimizerLate ===" | tee -a $LOG_FILE
export OBF_EARLY=1

for target in "${OBFS[@]}"; do
    export OBF_SUBST=0; export OBF_BOGUS=0; export OBF_FLATTEN=0
    export "$target"=1
    export LLVM_OBF_SCALAROPTIMIZERLATE_PASSES="${PASS_MAP[$target]}" 
    export LLVM_OBF_OPTIMIZERLASTEP_PASSES=""
    run_bench "$target" "$CURRENT_PLUGIN"
done

export OBF_SUBST=1; export OBF_BOGUS=1; export OBF_FLATTEN=1; 
export LLVM_OBF_OPTIMIZERLASTEP_PASSES="flattening,bogus,substitution"
run_bench "ALL_OBFS" "$CURRENT_PLUGIN"

}

run_for_plugin $THESIS_PLUGIN

export OBF_DEBUG=1
export OBF_SUBST=0
export OBF_BOGUS=0
export OBF_FLATTEN=0
export OBF_EARLY=0
export LLVM_OBF_SCALAROPTIMIZERLATE_PASSES=""
export LLVM_OBF_OPTIMIZERLASTEP_PASSES=""

run_for_plugin $VALIDATION_PLUGIN


echo "All benchmarks completed."