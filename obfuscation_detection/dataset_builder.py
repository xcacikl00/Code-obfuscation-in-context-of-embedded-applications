# @filename: dataset_builder_cmake.py
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

OUTPUT_CLEAN = "./dataset/clean"
OUTPUT_OBFUSCATED = "./dataset/obfuscated"
SYSROOT = "/usr/lib/arm-none-eabi"
OUTPUT_DIR = OUTPUT_CLEAN  # Change to OUTPUT_OBFUSCATED for obfuscated binaries
SOURCE_PATH = "./dataset/source"
OPT_FLAGS = ["-O0", "-O1", "-O2", "-Ofast", "-Os"]
PASS_FLAGS = [
        "flattening", 
        "bogus", 
        "substitution", 
        "split-basic-blocks",
    ]

C_TEMPLATE = """
    typedef int (*generic_func)();

    extern int {func_name}(); 

    void _start() {{ 
      volatile int keep =  ((generic_func){func_name})(); 
        while(1);
    }}
    """
    
PLUIGN_PATH = "../obfuscation_pass/obfuscator-llvm/build/libLLVMObfuscator.so"

COMMON_FLAGS = [
    "clang",
    "--target=arm-none-eabi",
    f"--sysroot={SYSROOT}",
    "-mcpu=cortex-m4",
    "-mthumb",
    "-ffunction-sections",    
    "-fdata-sections",       
    "-Wl,--strip-all",       
    "-Wl,--gc-sections",    
    "-DARM_MATH_DSP",
    "-fuse-ld=lld",
    "-lgcc",
    "-lnosys",
    "-L/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7e-m+fp/hard",
    f"-fpass-plugin={PLUIGN_PATH}" # test
    
    
    

]



import os
import subprocess
import json
import re
import shutil
import glob
from pathlib import Path
os.chdir(Path(__file__).resolve().parent)



def compile_function(func_name, opt_flag, pass_flag, library_path):
    """
    Compile a single function harness and link against a pre-built library.
    
    Args:
        func_name: Function name to compile
        opt_flag: Optimization flag (e.g., "-O2")
        pass_flag: Obfuscation pass flag (e.g., "flattening") or empty string for no obfuscation
        library_path: Path to pre-built static library
    
    Returns:
        bool: True if compilation succeeded, False otherwise
    """
    # check if library exists
    if not os.path.exists(library_path):
        print(f"  Error: Library not found at {library_path}")
        return False
    
    
    
    skeleton_path = create_skeleton(func_name)
    output_name = get_output_name(func_name, opt_flag, pass_flag)
    
    output_dir = OUTPUT_CLEAN if not pass_flag else OUTPUT_OBFUSCATED
    output_path = os.path.join(output_dir, output_name)
    
    if  os.path.exists(output_path): # already compiled
        os.remove(skeleton_path)
        return
    
    os.makedirs(output_dir, exist_ok=True)
    
    # build compile command - link against library instead of compiling sources
    cmd = COMMON_FLAGS + [opt_flag] + [skeleton_path, "-o", output_path, library_path]
    
    env = os.environ.copy()
    env["LLVM_OBF_OPTIMIZERLASTEP_PASSES"] = pass_flag
    
    result = subprocess.run(cmd, capture_output=True, text=True, env=env)
    
    os.remove(skeleton_path)
    
    if result.returncode != 0:
        print(f"  Error compiling {func_name} with opt={opt_flag} pass={pass_flag}: {result.stderr[:100]}")
        return False
    
    return True

def create_skeleton(func_name):  
    skeleton = f"temp_{func_name}.c"
    with open(skeleton, "w") as f:
        f.write(C_TEMPLATE.format(func_name=func_name))
    return skeleton


def build_library(library_name, opt_flag, pass_flag, source_files, include_flags, lib_output_dir="./libraries"):
    """
    Build a static library from source files with given compilation flags.
    
    Args:
        library_name: Base name for library (e.g., "CMSIS-NN")
        opt_flag: Optimization flag (e.g., "-O2")
        pass_flag: Obfuscation pass flag (e.g., "flattening") or empty string
        source_files: List of source files to compile
        include_flags: List of include path flags
        lib_output_dir: Directory to store libraries
    
    Returns:
        str: Path to created library, or None if failed
    """
    os.makedirs(lib_output_dir, exist_ok=True)
    
    lib_filename = f"{library_name}_{opt_flag.lstrip('-')}_{pass_flag}.a"
    lib_path = os.path.join(lib_output_dir, lib_filename)
    
    if os.path.exists(lib_path):
        return lib_path
    
    temp_dir = f"./temp_objs_{library_name}_{opt_flag}"
    os.makedirs(temp_dir, exist_ok=True)
    
    temp_objects = []
    
    for src in source_files:
        # compile source file
        obj_name = os.path.basename(src).replace('.c', '.o')
        obj_path = os.path.join(temp_dir, obj_name)
        
        compile_cmd = [
            "clang",
            "--target=arm-none-eabi",
            f"--sysroot={SYSROOT}",
            "-mcpu=cortex-m4",
            "-mthumb",
            "-c",
            opt_flag,
            "-ffunction-sections",
            "-fdata-sections",
            "-DARM_MATH_DSP",
            f"-fpass-plugin={PLUIGN_PATH}", # test
            "-L/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7e-m+fp/hard"

        ] + include_flags + [src, "-o", obj_path]
        
        env = os.environ.copy()
        env["LLVM_OBF_OPTIMIZERLASTEP_PASSES"] = pass_flag
        
        result = subprocess.run(compile_cmd, capture_output=True, text=True, env=env)
        if result.returncode != 0:
            print(f"    Error compiling {os.path.basename(src)}: {result.stderr[:80]}")
            shutil.rmtree(temp_dir)
            return None
        
        temp_objects.append(obj_path)
    
    # create library
    ar_cmd = ["llvm-ar", "rcs", lib_path] + temp_objects
    result = subprocess.run(ar_cmd, capture_output=True, text=True)
    
    shutil.rmtree(temp_dir)
    
    if result.returncode != 0:
        print(f"  Error creating library {lib_filename}: {result.stderr[:100]}")
        return None
    
    return lib_path


def get_commands_for_source(source_path):
    """
    Extract source files and include paths from CMake compile_commands.json
    
    Args:
        source_path: Path to source library (assumes compile_commands.json in source_path/build)
    
    Returns:
        dict with 'sources' (list of .c files) and 'includes' (list of -I flags)
    """
    compile_commands_path = Path(source_path) / "build" / "compile_commands.json"
    
    if not compile_commands_path.exists():
        raise FileNotFoundError(f"compile_commands.json not found at {compile_commands_path}")
    
    with open(compile_commands_path) as f:
        commands = json.load(f)
    
    sources = set()
    includes = set()
    
    for entry in commands:
        # extract source 
        file_path = entry.get('file', '')
        if file_path.endswith('.c'):
            sources.add(file_path)
        
        # extract include paths
        command = entry.get('command', '')
        include_matches = re.findall(r'-I(\S+)', command)
        for match in include_matches:
            includes.add(f"-I{match}")
    
    return {
        'sources': sorted(sources),
        'includes': sorted(includes)
    }

def get_output_name(func_name, opt_flag, pass_flag):
    return f"{func_name}_{opt_flag}_{pass_flag}.elf"
    


def compile_source(functions_list, source_path, lib_output_dir="./libraries"):
    """
    Compile harness files for given functions against pre-built libraries.
    First builds static libraries for all expected optimization/obfuscation flag combinations,
    then compiles function harnesses that link against these libraries.
    
    Args:
        functions_list: List of function names to compile
        source_path: Path to source library directory
        library_name: Base name for generated libraries
        lib_output_dir: Directory to store libraries
    """
    # get compilation info from source cmake
    # assumes file is present in /build
    
    source_files = []
    include_flags = []

    try:
        source_commands = get_commands_for_source(source_path)
        

    except FileNotFoundError:
        source_commands = None
        
        
    
    if source_commands:
        source_files = source_commands['sources']
        include_flags = source_commands['includes']
        
    else:
        exclude_dirs = {'test', 'tests', 'doc', 'docs', 'example', 'examples', 'externals', 'build', "tools"}

        search_pattern = os.path.join(source_path, "**", "*.c")
        
        source_files = []
        for f in glob.glob(search_pattern, recursive=True):
            abs_path = os.path.abspath(f)
            path_parts = set(abs_path.lower().split(os.sep))
            if not (path_parts & exclude_dirs): # set instersection
                source_files.append(abs_path)

        include_flags = [f"-I{os.path.abspath(source_path)}"]
        for root, dirs, files in os.walk(source_path):
            dirs[:] = [d for d in dirs if d.lower() not in exclude_dirs]
            for d in dirs:
                include_flags.append(f"-I{os.path.abspath(os.path.join(root, d))}")
    
        
    library_name = os.path.basename(source_path)
    
    print(f"Extracted {len(source_files)} source files and {len(include_flags)} include paths\n")
    
    print(f"Building libraries...")
    libraries = {}  # dict (opt_flag, pass_flag) : library_path
    
    print(f"  Building clean libraries with optimization flags...")
    for opt_flag in OPT_FLAGS:
        lib_path = build_library(library_name, opt_flag, "", source_files, include_flags, lib_output_dir)
        if lib_path:
            libraries[(opt_flag, "")] = lib_path
    
    print(f"  Building obfuscated libraries with individual passes at -O2...")
    for pass_flag in PASS_FLAGS:
        lib_path = build_library(library_name, "-O2", pass_flag, source_files, include_flags, lib_output_dir)
        if lib_path:
            libraries[("-O2", pass_flag)] = lib_path
    
    print(f"  Building obfuscated libraries with all passes...")
    all_passes = ",".join(PASS_FLAGS)
    for opt_flag in OPT_FLAGS[1:]:  # Skip -O0 for all passes
        lib_path = build_library(library_name, opt_flag, all_passes, source_files, include_flags, lib_output_dir)
        if lib_path:
            libraries[(opt_flag, all_passes)] = lib_path
    
    print(f"\nLibraries ready: {len(libraries)} variants\n")
    
    print(f"Compiling {len(functions_list)} functions...")
    for func in functions_list:
        print(f"  {func}...")
        
        # each opt flag, no obfuscation
        for opt_flag in OPT_FLAGS:
            if (opt_flag, "") in libraries:
                compile_function(func, opt_flag, "", libraries[(opt_flag, "")])
            else:
                print(f"unexpected combination of  flags {(opt_flag, "")}, library not present")
        
        
        # individual obf passses, O2
        for pass_flag in PASS_FLAGS:
            if ("-O2", pass_flag) in libraries:
                compile_function(func, "-O2", pass_flag, libraries[("-O2", pass_flag)])
            else:
                print(f"unexpected combination of  flags {("-O2", pass_flag)}, library not present")
        
        # 
        all_passes = ",".join(PASS_FLAGS)
        for opt_flag in OPT_FLAGS[1:]:
            if (opt_flag, all_passes) in libraries:
                compile_function(func, opt_flag, all_passes, libraries[(opt_flag, all_passes)])
            else:
                print(f"unexpected combination of  flags {(opt_flag, all_passes)}, library not present")
                
                
        
        


        
def main():
    # compile cmsis nn functions
    print("Compiling CMSIS-NN functions...")
    
    with open("CMSIS-NN_functions.csv", "r") as f:
        FUNCTIONS = [line.strip() for line in f.readlines()]
    
    compile_source(FUNCTIONS, "dataset/source/CMSIS-NN")

    with open("monocypher_functions.csv", "r") as f:
        FUNCTIONS = [line.strip() for line in f.readlines()]
    
    compile_source(FUNCTIONS, "dataset/source/monocypher")
    
    FUNCTIONS = ["kiss_fft_alloc","kiss_fft","kiss_fft_stride"]
    compile_source(FUNCTIONS, "dataset/source/kissfft")

    
    
if __name__ == "__main__":
    main()