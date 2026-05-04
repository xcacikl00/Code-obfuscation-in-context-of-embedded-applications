	.text
	.file	"semantic_test.c"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset %rbx, -16
	leaq	.L.str(%rip), %rbx
	movq	%rbx, %rdi
	movl	$-1, %esi
	xorl	%edx, %edx
	movl	$1, %ecx
	xorl	%eax, %eax
	callq	printf@PLT
	movq	%rbx, %rdi
	xorl	%esi, %esi
	movl	$1, %edx
	movl	$55, %ecx
	xorl	%eax, %eax
	callq	printf@PLT
	movq	%rbx, %rdi
	xorl	%esi, %esi
	movl	$5, %edx
	movl	$10, %ecx
	xorl	%eax, %eax
	callq	printf@PLT
	leaq	.L.str.1(%rip), %rdi
	movl	$-1, %esi
	xorl	%edx, %edx
	movl	$1, %ecx
	movl	$2, %r8d
	movl	$3, %r9d
	xorl	%eax, %eax
	callq	printf@PLT
	movq	%rbx, %rdi
	movl	$5, %esi
	movl	$-1, %edx
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	callq	printf@PLT
	movq	%rbx, %rdi
	xorl	%esi, %esi
	movl	$12, %edx
	movl	$30, %ecx
	xorl	%eax, %eax
	callq	printf@PLT
	movq	%rbx, %rdi
	movl	$1, %esi
	xorl	%edx, %edx
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	callq	printf@PLT
	leaq	.L.str.2(%rip), %rdi
	movl	$2, %esi
	movl	$-1, %edx
	xorl	%eax, %eax
	callq	printf@PLT
	movl	$7, %edi
	callq	fib
	movl	%eax, %ebx
	movl	$10, %edi
	callq	fib
	leaq	.L.str.3(%rip), %rdi
	xorl	%esi, %esi
	movl	$1, %edx
	movl	%ebx, %ecx
	movl	%eax, %r8d
	xorl	%eax, %eax
	callq	printf@PLT
	leaq	.L.str.4(%rip), %rdi
	leaq	.L.str.5(%rip), %rsi
	leaq	.L.str.8(%rip), %rdx
	leaq	.L.str.11(%rip), %rcx
	xorl	%eax, %eax
	callq	printf@PLT
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function fib
	.type	fib,@function
fib:                                    # @fib
	.cfi_startproc
# %bb.0:
	pushq	%r14
	.cfi_def_cfa_offset 16
	pushq	%rbx
	.cfi_def_cfa_offset 24
	pushq	%rax
	.cfi_def_cfa_offset 32
	.cfi_offset %rbx, -24
	.cfi_offset %r14, -16
	movl	%edi, %r14d
	xorl	%ebx, %ebx
	cmpl	$2, %edi
	jge	.LBB1_2
# %bb.1:
	movl	%r14d, %ecx
	jmp	.LBB1_4
.LBB1_2:
	xorl	%ebx, %ebx
	.p2align	4, 0x90
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	leal	-1(%r14), %edi
	callq	fib
	leal	-2(%r14), %ecx
	addl	%eax, %ebx
	cmpl	$3, %r14d
	movl	%ecx, %r14d
	ja	.LBB1_3
.LBB1_4:
	addl	%ecx, %ebx
	movl	%ebx, %eax
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end1:
	.size	fib, .Lfunc_end1-fib
	.cfi_endproc
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"%d %d %d\n"
	.size	.L.str, 10

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"%d %d %d %d %d\n"
	.size	.L.str.1, 16

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"%d %d\n"
	.size	.L.str.2, 7

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"%d %d %d %d\n"
	.size	.L.str.3, 13

	.type	.L.str.4,@object                # @.str.4
.L.str.4:
	.asciz	"%s %s %s\n"
	.size	.L.str.4, 10

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"Sun"
	.size	.L.str.5, 4

	.type	.L.str.8,@object                # @.str.8
.L.str.8:
	.asciz	"Wed"
	.size	.L.str.8, 4

	.type	.L.str.11,@object               # @.str.11
.L.str.11:
	.asciz	"Sat"
	.size	.L.str.11, 4

	.ident	"Ubuntu clang version 18.1.3 (1ubuntu1)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
