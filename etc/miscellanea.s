.text

.global proper_intro
proper_intro:
    # Prepare our stack frame.
    pushq   %rbp
    movq    %rsp, %rbp
    # Make some room on the stack to save %rbx
    subq    $0x10, %rsp

    # %rbx = (uint8_t *) p
    leaq    0x10(%rbp), %rax
    movq    (%rax), %rbx

    # ...

.global proper_outro
proper_outro:
    # Release the stack frame.
    addq    $0x10, %rsp
    popq    %rbp
    retq

.global increment_data
increment_data:
    incb    (%rbx)

.global decrement_data
decrement_data:
    decb    (%rbx)

.global increment_ptr
increment_ptr:
    incq    %rbx

.global decrement_ptr
decrement_ptr:
    decq    %rbx

.global call_output
call_output:
    # Save %rbx
    movq    %rbx, -0x8(%rbp)

    # (first argument) = *p
    movb    (%rbx), %dl # %dl = *p
    movsbl  %dl, %edi

    # Load the address of the output function
    leaq    0x10(%rbp), %rax    # %rax = bf_runtime_context*
    movq    0x8(%rax), %rax     # %rax = output_byte*

    callq   *%rax

    # Restore %rbx
    movq    -0x8(%rbp), %rbx

