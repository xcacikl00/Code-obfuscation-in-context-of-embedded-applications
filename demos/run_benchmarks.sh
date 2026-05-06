OBFS=("OBF_SUBST" "OBF_BOGUS" "OBF_FLATTEN")

declare -A PASS_MAP
PASS_MAP["OBF_SUBST"]="substitution"
PASS_MAP["OBF_BOGUS"]="bogus"
PASS_MAP["OBF_FLATTEN"]="flattening"

run_bench() {
    local label=$1
        echo "Benchmarking with $1" | tee -a $LOG_FILE


    cd build
    make clean
    make -j$(nproc)
    cd ..
    
    bash -x ./benchmark.sh 
}

export OBF_DEBUG=1
export OBF_SUBST=0
export OBF_BOGUS=0
export OBF_FLATTEN=0
export LLVM_OBF_SCALAROPTIMIZERLATE_PASSES=""

run_bench "NO_OBFS"

for target in "${OBFS[@]}"; do
    export OBF_SUBST=0
    export OBF_BOGUS=0
    export OBF_FLATTEN=0
    
    export "$target"=1
    export LLVM_OBF_SCALAROPTIMIZERLATE_PASSES="${PASS_MAP[$target]}"
    
    run_bench "$target"
done

export OBF_SUBST=1
export OBF_BOGUS=1
export OBF_FLATTEN=1
export LLVM_OBF_SCALAROPTIMIZERLATE_PASSES="flattening, bogus, substitution"

run_bench "ALL_OBFS"

echo "All benchmarks completed."