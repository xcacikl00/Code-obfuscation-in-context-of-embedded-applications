/*
 * @filename: benchmark.sh
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

#!/bin/bash

PROGRAMMER="/mnt/c/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"
LOG_FILE="benchmark_results.log"


DEMOS=(
    "PID|build/demo_pid/PID.elf"
    "FFT|build/demo_fft/FFT.elf"
    "AES|build/demo_aes/AES.elf"
)
CURRENT_PLUGIN=$(grep "PLUGIN_PATH:" build/CMakeCache.txt | cut -d'=' -f2)
echo "Path: $CURRENT_PLUGIN"


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

    SIZE_INFO=$(arm-none-eabi-size "$ELF_PATH" | tail -n 1)
    TEXT_SIZE=$(echo "$SIZE_INFO" | awk '{print $1}')
    DATA_SIZE=$(echo "$SIZE_INFO" | awk '{print $2}')
    SIZE_BYTES=$((TEXT_SIZE + DATA_SIZE))

    #  flash the binary
    echo "  Flashing..."
    "$PROGRAMMER" -c port=SWD -w "$ELF_PATH" -v -rst > /dev/null 2>&1

    # wait for the program to run 
    echo "  Executing (0.5s)..."
    sleep 0.5

    # read memory
    RESULT=$("$PROGRAMMER" -c port=SWD mode=Hotplug -r32 "$ADDR" 1)

    # parse stm32 programmer result
    HEX_VAL=$(echo "$RESULT" | grep -i "^$ADDR" | awk -F': ' '{print $2}' | tr -d '\r' | xargs)
    if [ -z "$HEX_VAL" ]; then
        echo "  Result: [FAILED TO READ]" | tee -a $LOG_FILE
    else
        DEC_VAL=$((16#$HEX_VAL))
        printf "%-10s | Addr: %s | Hex: 0x%-8s | Dec: %-10d | Size: %d bytes\n" \
                    "$NAME" "$ADDR" "$HEX_VAL" "$DEC_VAL" "$SIZE_BYTES" | tee -a $LOG_FILE    
    fi
done

echo "------------------------------------------------" | tee -a $LOG_FILE
