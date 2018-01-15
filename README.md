# apportable

**This project is not finished yet; use with care.**

C library to help making applications portable.

## Motivation

There is a distinction between portable applications and portable source-code;
the latter is source code with an inherent flexibility which lets us compile
the program on different platforms The former instead allows to copy
applications from one similar system to another, without the need to place
dependencies in system-default, and thus particularly guarded locations -
i.e. without installation. This library is about offering the functionality
to make binaries portable, providing itself portable source-code, so that it
is particularly interesting for applications with already have portable source
code.

Installed applications can easily locate their files on absolute paths,
once they know the distributions conventions. Configuration files for example are
usually considered to be under **$(sysconfdir)**, which the maintainer can define
at compile time to be - usually - **/etc/**. Portable applications instead can not
be accommodated with a fixed absolute path.

Different strategies exist, but all depend on a carefully prepared environment,
which is normally not realistic. Neither environment variables are reliably set,
nor can a regular user set the working directory for a binary.

Portable applications need a way to compute the paths to their resources
relative to the binary file. This is surprisingly a hard problem, especially
cross-platform. The code differs not only by platform, but also for the
compiler and the specific C library implementation.


## Architecture

This library is designed such that existing build systems can be leveraged, without
too much code change. It is also designed to be turned off by default, so that adding
apportable to the build system in not enough to introduce additional security problems,
except if requested explicitely by the maintainers.

To use apportable add `apportable.c` to the linked objects, and somehow add `apportable.h` to
the necessary source files. It should not hurt to add `apportable.h` globally, as
all variables are prefixed with `apportable_`. Finally you need to enable the code
explicitely by defining `APPORTABLE` on the compiler.

Once you have integrated apportable in the build and enabled it with APPORTABLE,
the basic idea is that whenever the build system expects you to define a static path

```Makefile
CONFIGFILE = "/etc/apportable.conf"
```

it can be replaced by a function call:

```Makefile
CONFIGFILE = apportable_template("$ORIGIN/../etc/apportable.conf")
```

This is possible as static strings are basically pointers to an array of characters (`char *`),
thus replacing the code with a function which is guaranteed to return a valid pointer
is normally fine.

**Note**: this works, as long as the string is not concatenated to other strings:

```c
FILE * cf = open(PREFIX CONFIGFILE, "r");
```

In such a case, the source code needs to be modified directly:

```c
FILE * cf = open(apportable_template("$ORIGIN/../etc/apportable.conf"), "r");
```

## Source code compatibility

This should be written in POSIX 2008 compatible C99, to make it interesting
and usable by most existing software.

It was tested to compile on:

 * Apple LLVM version 9.0.0
 * gcc (Debian 6.3.0-18) 6.3.0
 * Visual Studio 2008

For testig (and a programming exercise in handling Unicode and Python), a
Python extension has been added, which is tested with the same compilers and
Python 2.7 and Python 3.2+.


## Encoding (UTF-8, UTF-16, UTF-32)

Functions for both ASCII compatible UTF-8 (`char *`) and native *wide
character* (`wchar_t`, UTF-16, UTF-32) arguments are provided, the latter are
prefixed with a `w` (like in `wutf8`). All the necessary conversions are done
with platform standard means, that is for now either libiconv or the Windows API.

The testing infrastructure cross-checks the internal conversion against
Python's conversion between the formats, in the hope that this further
validates the function's design.

