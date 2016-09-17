.text

# Stack layout
#
#        |----------------|
#   0x20 | *input_byte    |
#        |----------------|
#   0x18 | *output_byte   |
#        |----------------|
#   0x10 | *universe      |
#        |----------------|
#   0x08 | [padding]      |
#        |----------------|
#   %rbp | prev. bp       |
#        |----------------|
#  -0x08 | save %rbx      |
#        |----------------|
#  -0x10 | [padding]      |
#        |----------------|
#


.global proper_intro
proper_intro:
    # Prepare our stack frame.
    pushq   %rbp
    movq    %rsp, %rbp

    # Save %rbx on the stack.
    subq    $0x10, %rsp
    movq    %rbx, -0x08(%rbp)

    # %rbx = (uint8_t *) p
    leaq    0x10(%rbp), %rbx
    # %rax = *p
    movq    (%rbx), %rax

    # ...

.global proper_outro
proper_outro:
    # Restore %rbx
    movq    -0x08(%rbp), %rbx

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
    # (first argument) = *p
    movb    (%rbx), %dl # %dl = *p
    movsbl  %dl, %edi

    # Load the address of the output function
    leaq    0x10(%rbp), %rax    # %rax = bf_runtime_context*
    movq    0x8(%rax), %rax     # %rax = output_byte*

    # Call bf_runtime_context.output_byte
    callq   *%rax

.global call_input
call_input:
    # Do indirect call input_byte()
    movb    $0x0, %al
    leaq    0x10(%rbp), %rcx
    callq   *0x10(%rcx)

    # The value is returned in %al

    # *p = %al
    movb    %al, (%rbx)


.global loop_skeleton
loop_skeleton:
    # Top of the loop

    # %cl = *p
    movb    (%rbx), %cl

    # if == 0, skip...
    cmpb    $0x0, %cl
    # Placeholder is 0xFFFFFFFF
    je      -0x1

    # loop body
    nop

    jmp -0x1

    # After.
    nop

