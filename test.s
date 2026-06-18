.text
.global main
main:
  pushq %rbp
  movq %rsp, %rbp
  subq $80, %rsp
  leaq .LC1(%rip), %rcx
  leaq -16(%rbp), %rdx
  movq $0, %rax
  callq scanf
  movl -16(%rbp), %eax
  movslq %eax, %rax
  movq %rax, -16(%rbp)
  movq $1, %rax
  movq %rax, -8(%rbp)
  movq $0, %rax
  movq %rax, -24(%rbp)
L1:
  movq -8(%rbp), %rax
  cmpq -16(%rbp), %rax
  setle %al
  movzbl %al, %eax
  movq %rax, -32(%rbp)
  movq -32(%rbp), %rax
  testq %rax, %rax
  jz L2
  movq -24(%rbp), %rax
  addq -8(%rbp), %rax
  movq %rax, -40(%rbp)
  movq -40(%rbp), %rax
  movq %rax, -24(%rbp)
  movq -8(%rbp), %rax
  addq $1, %rax
  movq %rax, -48(%rbp)
  movq -48(%rbp), %rax
  movq %rax, -8(%rbp)
  jmp L1
L2:
  movq -24(%rbp), %rcx
  callq print_int
  movq $0, %rax
  movq %rbp, %rsp
  popq %rbp
  retq
.data
.LC0: .string "%d\n"
.LC1: .string "%d"
.text
print_int:
  pushq %rbp
  movq %rsp, %rbp
  subq $32, %rsp
  movq %rcx, %rdx
  leaq .LC0(%rip), %rcx
  movq $0, %rax
  callq printf
  movq %rbp, %rsp
  popq %rbp
  retq
