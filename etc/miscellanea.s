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

