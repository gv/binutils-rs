# Playing with libfd and libopcodes

This directory contains examples that show howto call `binutils` bfd and
opcodes libraries from C:
1. [test_binary](examples/src/test_binary.c): this is the example from
   https://www.toothycat.net/wiki/wiki.pl?Binutils/libopcodes that was adapt to
   work with binutils 2.29.1
2. [test_buffer_x86_64](examples/src/test_buffer_x86_64.c): this example shows
   howto disassemble a buffer containing x86 instructions
3. [test_buffer_mep](examples/src/test_buffer_mep.c): similar to the second
   example with a more exotic architecture and binutils builtins

**THIS IS OUTDATED**
For convenience, the [libbfd](http://htmlpreview.github.com/?https://github.com/guedou/binutils-rs/blob/master/resources/docs/ToothyWiki_%20Binutils_Bfd.html) and
[libopcodes](http://htmlpreview.github.com/?https://github.com/guedou/binutils-rs/blob/master/resources/docs/ToothyWiki_%20Binutils_Libopcodes.html) documentation from
toothycat.net is archived in this repository.


## Building examples

The examples assume that binutils 2.43 is built at the root of this
repostitory. 

It can be compiled by doing `cargo build` at the root of this repository.
