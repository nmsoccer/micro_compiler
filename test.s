.section .data
	test_global: .int 0
	test_gc: .int 0
.section .rodata
	test_const1: .asciz "hello , world"
	test_const0: .ascii b
.section .bss
.section .text
.global _start , test_hello , test_gc , test_main
test_hello:
	pushl %ebp
	movl %esp , %ebp
	subl $24 , %esp

	movl test_gc , %eax
	movl -24(ebp) , %eax
	movl $3 , %eax
	xor eax , eax
	movb test_const0 , %al
	movl $test_const1 , %eax
	movl $0 , %eax

	movl %ebp , %esp
	pushl %ebp
	ret

	movl %ebp , %esp
	pushl %ebp
	ret
_start:
	pushl %ebp
	movl %esp , %ebp
	subl $16 , %esp

	movl $0 , %eax

	movl %ebp , %esp
	pushl %ebp
	ret

	movl %ebp , %esp
	pushl %ebp
	ret
