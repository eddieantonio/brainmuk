.text
.global increment
increment:
    incb (%rax)

.global decrement
decrement:
    decb (%rax)

