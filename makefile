make:all
	make build
	make run

make:build

	clang --target=arm-none-eabi -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -O0 -g -c main.c -o main.o
	clang --target=arm-none-eabi -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -O0 -g -c startup.c -o startup.o
	arm-none-eabi-gcc -T linker.ld main.o startup.o -o test.elf --specs=rdimon.specs -lc -lrdimon

make:run

	qemu-system-arm -cpu cortex-m4 -machine lm3s6965evb -nographic -semihosting -kernel test.elf
