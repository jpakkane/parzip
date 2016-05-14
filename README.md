# parzip

A command line utility to pack and unpack zip archives using multiple threads.

Licensed under GPLv3 or later.

## Supports

 - both zipping and unzipping
 - multithreading
 - uncompressed (i.e. stored) files
 - [deflate](http:zlib.net) and [lzma](http://7-zip.org/sdk.html) decompression
 - lzma compression
 - ZIP64 extensions (i.e. >4 GB files)
 - unix file attributes

## Does not support

 - modifying existing archives
 - encryption (zip encryption is broken, use GPG instead)
 - ancient compression methods
 - archives split to multiple files
 - VMS, Amiga or any other non-modern platform

## Contributions

Please send patches as merge requests via the [Github project](https://github.com/jpakkane/jzip).
