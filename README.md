Brainmuk
========

[![Build Status](https://travis-ci.org/eddieantonio/brainmuk.svg?branch=master)](https://travis-ci.org/eddieantonio/brainmuk)

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

