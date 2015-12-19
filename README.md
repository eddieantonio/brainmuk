Brainmuk
========

> brainfuck x86 compiler and interpreter

brainmuk interprets [brainfuck][] code by compiling directly into x86
machine code.

Usage
-----

    $ brainmuk file.bf

[brainfuck]: https://en.wikipedia.org/wiki/Brainfuck

Scripts
-------

You can write scripts as such:

    $ cat hello.bf
    #!/usr/bin/env brainmuk
    ++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.
    $ chmod +x hello.bf
    $ ./hello.bf
    Hello, World!

Brainfuck Instructions
======================

Any characters other than these instructions are ignored.

 - `>` increment data pointer by one `p++`
 - `<` decrement data pointer by one `p--`
 - `+` increment the byte at the data pointer `++*p`
 - `-` decrement the byte at the data pointer `++*p`
 - `.` output the byte at the data pointer `putchar(*p)`
 - `,` get a byte of input, placing at the data pointer `*p = getchar()`
 - `[` if the byte at data pointer is zero, branch to the _matching_ `[` instruction `while (*p) {`
 - `]` if the byte at the data pointer is non-zero, branch back to the _matching_ `[` instruction `}`

