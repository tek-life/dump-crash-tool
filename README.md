dump-crash-tool Introduction
=========

This tool is to compose an available image for Crash Utility, which could analyze crash ARM Linux Kernel.

Compile
=========
On x86 PC:
$make mkdump

Usage
=========
$./mkdump –cpu_notes_paddr=0Xxxx –vmcore_notes_paddr=0Xxxx memory-image

After this command excuting, _newvmcore_ is available.

