import subprocess
import os
from pathlib import Path

OUTPUT_CLEAN = "./dataset/clean"
OUTPUT_OBFUSCATED = "./dataset/obfuscated"
LIB_PATH = "./libcmsisnn.a"
os.makedirs(OUTPUT_CLEAN, exist_ok=True)
os.makedirs(OUTPUT_OBFUSCATED, exist_ok=True)
SYSROOT = "/usr/lib/arm-none-eabi"
OUTPUT_DIR = OUTPUT_CLEAN  # Change to OUTPUT_OBFUSCATED for obfuscated binaries

AUTOMOTIVE_PATH = "./dataset/source/mibench/automotive"
CMSIS_PATH = "./dataset/source"
PQM4_PATH = "./dataset/source/pqm4"
env_vars = {}
env_vars["LLVM_OBF_SCALAROPTIMIZERLATE_PASSES"] = ""
OPT_FLAG = "" 

COMMON_FLAGS = [
    "clang",
    "--target=arm-none-eabi",
    f"--sysroot={SYSROOT}",
    "-mcpu=cortex-m4",
    "-mthumb",
    "-DARM_MATH_DSP",
    "-fuse-ld=lld",
    "-lgcc",
    "-L/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7e-m+fp/hard",
    "-fpass-plugin=/home/gammut/obfuscator-llvm/build/libLLVMObfuscator.so", # test
    "-lm"

]



def compile_cmsis_nn():
    with open("CMSIS-NN_functions.csv", "r") as f:
        FUNCTIONS = [line.strip() for line in f.readlines()]

    C_TEMPLATE = """
    typedef void (*generic_func)();

    extern void {func_name}(); 

    void _start() {{ 
        ((generic_func){func_name})(); 
        while(1);
    }}
    """
    print(f"Starting cmsis-nn compilation... Total functions: {len(FUNCTIONS)}")
    successful = 0   
    
    for func in FUNCTIONS:
        skeleton = f"temp_{func}.c"
        
        with open(skeleton, "w") as f:
            f.write(C_TEMPLATE.format(func_name=func))    
            
        
        cmd = COMMON_FLAGS + [
            
            OPT_FLAG,
            f"-I{CMSIS_PATH}/CMSIS-NN-7.0.0/Include",
            f"-I{CMSIS_PATH}/CMSIS_6/CMSIS/Core/Include",
            skeleton,
            "-o", os.path.join(OUTPUT_DIR, f"{func + '_' +  OPT_FLAG + '_' + env_vars['LLVM_OBF_SCALAROPTIMIZERLATE_PASSES']}.elf"),
            LIB_PATH,
        ]
        #debug        
        # print(f"compiling {func} with command {' '.join(cmd)}")

        
        result = subprocess.run(cmd, capture_output=True, text=True, env=env_vars)
        
        
        if result.returncode != 0:
            print(f"compilation error {func}: {result.stderr}")
        
        os.remove(skeleton)
        successful += 1

    print(f"cmsis-nn compiled with {successful} functions")
    
    


#automotive compile
def get_include_paths(c_file_path):
        file_dir = os.path.dirname(c_file_path)
        return [f"-I{file_dir}"]

def compile_c_file(c_file_path, output_elf_path):
    include_paths = get_include_paths(c_file_path)
    
    cmd = COMMON_FLAGS + include_paths + [OPT_FLAG,
        c_file_path,
        "-o", output_elf_path
    ]
    
    #debug
    # print(f"compiling {c_file_path} with command {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True, env=env_vars)
    return result.returncode == 0, result.stderr


def compile_automotive():


    c_files = list(Path(AUTOMOTIVE_PATH).rglob("*.c"))

    successful = 0
    failed = 0

    for c_file in c_files:
        file_stem = c_file.stem
        output_name = f"{file_stem}"
        output_path = os.path.join(OUTPUT_DIR, output_name,)
        output_path += "_" + OPT_FLAG + "_" + env_vars["LLVM_OBF_SCALAROPTIMIZERLATE_PASSES"] + ".elf"
        output, stderr = compile_c_file(str(c_file), output_path)
        if stderr:
            # print(f"Compilation error: {stderr.strip()}")
            pass
        
        if output == 0:
            successful += 1
        else:
            failed += 1

    print(f"automotive compilation complete")
    print(f"successful: {successful}")
    print(f"failed: {failed}")
    #todo fix failures
    print("currently failures are exptected due to automotive using stdlib which is not linked files need to bo modified to not use stdlib")
    
    
    

def main():
    print(f"Output directory: {OUTPUT_CLEAN}")
    
    
    # env_vars["LLVM_OBF_SCALAROPTIMIZERLATE_PASSES"] = "flattening, bogus, substitution, split-basic-blocks"
    opt_flags = ["-O0", "-O1", "-O2", "-Ofast", "-Os"]
    pass_flags = [
        "flattening", 
        "bogus", 
        "substitution", 
        "split-basic-blocks",
    ]
    
    all_passes = ",".join(pass_flags)
    
    
    for flag in opt_flags:
        global OPT_FLAG
        OPT_FLAG = flag
        print(f"\nCompiling with optimization flag: {OPT_FLAG}")
        compile_cmsis_nn()
        compile_automotive()
        
        
    print("compiling obfuscated binaries")
    global OUTPUT_DIR
    OUTPUT_DIR = OUTPUT_OBFUSCATED
    
    OPT_FLAG = "-O2"
    for pass_flag in pass_flags:
        env_vars["LLVM_OBF_SCALAROPTIMIZERLATE_PASSES"] = pass_flag
        print(f"  Using obfuscation passes: {pass_flag}")
        compile_cmsis_nn()
        compile_automotive()
        
    
    env_vars["LLVM_OBF_SCALAROPTIMIZERLATE_PASSES"] = all_passes
    for flag in opt_flags:
        OPT_FLAG = flag
        compile_cmsis_nn()
        compile_automotive()


    
    print(f"clean elf files: {len([f for f in os.listdir(OUTPUT_CLEAN) if f.endswith('.elf')])}")
    print(f"obfuscated elf files: {len([f for f in os.listdir(OUTPUT_OBFUSCATED) if f.endswith('.elf')])}")

    
if __name__ == "__main__":
    main()




