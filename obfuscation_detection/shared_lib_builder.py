import os
import subprocess
import glob
from tqdm import tqdm

# configuration
src_dir = "./dataset/source/CMSIS-NN-7.0.0/Source"
inc_nn = "./dataset/source/CMSIS-NN-7.0.0/Include"
inc_core = "./dataset/source/CMSIS_6/CMSIS/Core/Include"
obj_dir = "build_lib"
lib_name = "libcmsisnn.a"

os.makedirs(obj_dir, exist_ok=True)

# build static library
c_files = glob.glob(f"{src_dir}/**/*.c", recursive=True)
for c_file in tqdm(c_files, desc="CMSIS-NN compilation"):
    obj_file = os.path.join(obj_dir, os.path.basename(c_file).replace(".c", ".o"))
    subprocess.run([
        "arm-none-eabi-gcc", "-mcpu=cortex-m4", "-mthumb", "-O2",
        "-DARM_MATH_DSP",
        "-I", inc_nn, "-I", inc_core,
        "-c", c_file, "-o", obj_file
    ], check=True)

subprocess.run(f"arm-none-eabi-ar rcs {lib_name} {obj_dir}/*.o", shell=True, check=True)


def build_mbedtls_static_lib(mbed_path, output_path):
    src_files = glob.glob(os.path.join(mbed_path, "library", "*.c"))
    obj_files = []

    for src in src_files:
        obj = src.replace(".c", ".o")
        cmd = [
            "clang", "-c", "--target=arm-none-eabi", 
            "-mcpu=cortex-m4", "-mthumb", "-mfloat-abi=hard",
            f"-I{os.path.join(mbed_path, 'include')}",
            "-O2", src, "-o", obj
        ]
        subprocess.run(cmd, check=True)
        obj_files.append(obj)

    # create the .a file
    ar_cmd = ["llvm-ar", "rcs", output_path] + obj_files
    subprocess.run(ar_cmd, check=True)
    
    # cleanup .o files
    for obj in obj_files:
        os.remove(obj)

build_mbedtls_static_lib("./dataset/source/mbedtls", "build_lib/libmbedtls.a")