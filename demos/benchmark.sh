#!/bin/bash

PROGRAMMER="/mnt/c/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"
LOG_FILE="benchmark_results.log"


DEMOS=(
    "PID|build/demo_pid/PID.elf"
    "FFT|build/demo_fft/FFT.elf"
    "AES|build/demo_aes/AES.elf"
)

echo "Target Device: STM32F411 (Black Pill)" | tee -a $LOG_FILE
echo "------------------------------------------------" | tee -a $LOG_FILE

for ENTRY in "${DEMOS[@]}"; do
    NAME="${ENTRY%%|*}"
    ELF_PATH="${ENTRY#*|}"

    echo "Processing $NAME..."
    if [ ! -f "$ELF_PATH" ]; then
        echo "Error: $ELF_PATH not found. Skipping..." | tee -a $LOG_FILE
        continue
    fi

    #  Eectract the adress of the benchmark variable
    RAW_ADDR=$(arm-none-eabi-nm -S "$ELF_PATH" | grep "benchmark_result" | awk '{print $1}')
    
    if [ -z "$RAW_ADDR" ]; then
        echo "Error: benchmark_result symbol not found in $NAME" | tee -a $LOG_FILE
        continue
    fi
    
    ADDR="0x$RAW_ADDR"

    #  flash the binary
    echo "  Flashing..."
    "$PROGRAMMER" -c port=SWD -w "$ELF_PATH" -v -rst > /dev/null 2>&1

    # wait for the program to run 
    echo "  Executing (2s)..."
    sleep 2

    # read memory
    RESULT=$("$PROGRAMMER" -c port=SWD mode=Hotplug -r32 "$ADDR" 1)

    # parse stm32 programmer result
    HEX_VAL=$(echo "$RESULT" | grep -i "^$ADDR" | awk -F': ' '{print $2}' | tr -d '\r' | xargs)
    if [ -z "$HEX_VAL" ]; then
        echo "  Result: [FAILED TO READ]" | tee -a $LOG_FILE
    else
        DEC_VAL=$((16#$HEX_VAL))
        printf "%-10s | Addr: %s | Hex: 0x%-8s | Dec: %d\n" "$NAME" "$ADDR" "$HEX_VAL" "$DEC_VAL" | tee -a $LOG_FILE
    fi
done

echo "------------------------------------------------" | tee -a $LOG_FILE
echo "Benchmark Complete." | tee -a $LOG_FILE