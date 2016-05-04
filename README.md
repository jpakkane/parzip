# jzip

A command line utility to expand zip archives.

Licensed under GPLv3 or later.

## Supports

 - multithreading
 - uncompressed (i.e. stored) files
 - [deflate](http:zlib.net) and [lzma](http://7-zip.org/sdk.html) compressed files
 - ZIP64 extensions (i.e. >4 GB files)
 - unix file attributes

## Does not support

 - encryption (zip encryption is broken, use GPG instead)
 - ancient compression methods
 - archives split to multiple files
 - VMS, Amiga or any other non-modern platform
 - CRC checksums are not validated

## Maybe supports in the future

 - creating ZIP files

## Contributions

Please send patches as merge requests via the [Github project](https://github.com/jpakkane/jzip).
