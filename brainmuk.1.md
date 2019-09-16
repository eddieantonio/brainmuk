% BRAINMUK(1) | Brainmuk

NAME
====

**brainmuk** — brainfuck x86 compiler and interpreter

SYNOPSIS
========

| **brainmuk** \[**-m**|**-\-universe-size**=*size*[k|m|g]] \[_file_]
| **brainmuk** \[**-\-help**|**-\-version**]

DESCRIPTION
===========

**brainmuk** interprets the brainfuck language by compiling source into
x86-64 machine code. Either provide a file to interpret or invoke with
no arguments to start the read-eval-(maybe)print-loop (REPL).

Options
-------

-h, -\-help

:   Prints brief usage information.

-m *size*, -\-universe-size=*size*

:   The size of brainmuk's memory, in megabytes. Technically,
    a brainfuck program should have an infinite memory; however,
    **brainmuk**, requests that you set the size. If not specified, the
    default size is 640 KiB, which oughta be enough for most purposes.

    Suffix *size* with **m** for megabytes, **g** for gigabytes, or even
    **k** for kilobytes.

-v, -\-version

:   Prints the current version number.

Language Variety
================

 - Each **cell** is an **unsigned 8 bit integer**.
 - There are a **finite** amount of cells (specified by the
   **-\-universe-size** option).
 - When the input operator (`,`) receives an **end-of-file** it returns
   **0xFF**. 0xFF is always an invalid ASCII and UTF-8 byte, and is easy
   to compare.

BUGS
====

See GitHub Issues: <https://github.com/eddieantonio/brainmuk/issues>

AUTHOR
======

Eddie Antonio Santos <easantos@ualberta.ca>

COPYRIGHT
=========

2015, 2019 (C) Eddie Antonio Santos. MIT Licensed.
