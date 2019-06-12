## Howto: cross-compiling tinc for Windows under Linux using MinGW

This howto describes how to create a Windows binary of tinc. Although it is possible to compile tinc under Windows itself, cross-compiling it under Linux is much faster. It is also much easier to get all the dependencies in a modern distribution. Therefore, this howto deals with cross-compiling tinc with MinGW under Linux on a Debian distribution.

The result is a 32-bit executable. If you want to create a 64-bit executable, have a look at the [64-bit cross-compilation example](http://tinc-vpn.org/examples/cross-compiling-64-bit-windows-binary/).

### Overview

The idea is simple:

- Install MinGW and Wine.
- Create a directory where we will perform all cross-compilations.
- Get all the necessary sources.
- Cross-compile everything.

### Installing the prerequisites for cross-compilation

There are only a few packages that need to be installed as root to get started:

```
sudo apt-get install mingw-w64 wine git-core quilt
sudo apt-get build-dep tinc
```

Other Linux distributions may also have MinGW packages, use their respective package management tools to install them. Debian installs the cross-compiler in `/usr/i686-w64-mingw32/`. Other distributions might install it in another directory however, for example `/usr/i686-pc-mingw32/`. Check in which directory it is installed, and replace all occurences of `i686-w64-mingw32` in this example with the correct name from your distribution.

### Setting up the build directory and getting the sources

We will create a directory called `mingw/` in the home directory. We use apt-get to get the required libraries necessary for tinc, and use `git` to get the latest development version of tinc.

```
mkdir $HOME/mingw
cd $HOME/mingw
apt-get source openssl liblzo2-dev zlib1g-dev ncurses
git clone git://tinc-vpn.org/tinc
```

### Making cross-compilation easy

To make cross-compiling easy, we create a script called `mingw` that will set up the necessary environment variables so configure scripts and Makefiles will use the MinGW version of GCC and binutils:

```
mkdir $HOME/bin
cat >$HOME/bin/mingw << 'EOF'
#!/bin/sh
PREFIX=i686-w64-mingw32
export CC=$PREFIX-gcc
export CXX=$PREFIX-g++
export CPP=$PREFIX-cpp
export RANLIB=$PREFIX-ranlib
export PATH="/usr/$PREFIX/bin:$PATH"
exec "$@"
EOF
chmod u+x $HOME/bin/mingw
```

If `$HOME/bin` is not already part of your `$PATH`, you need to add it:

```
export PATH="$HOME/bin:$PATH"
```

We use this script to call `./configure` and `make` with the right environment variables, but only when the `./configure` script doesn’t support cross-compilation itself. You can also run the export commands from the `mingw` script by hand instead of calling the mingw script for every `./configure` or `make` command, or execute `$HOME/bin/mingw $SHELL` to get a shell with these environment variables set, but in this howto we will call it explicitly every time it is needed.

### Compiling LZO

Cross-compiling LZO is easy:

```
cd $HOME/mingw/lzo2-2.08
./configure --host=i686-w64-mingw32
make
make DESTDIR=$HOME/mingw install
```

### Compiling Zlib

Cross-compiling Zlib is also easy, but a plain `make` failed to compile the tests, so we only build the static library here:

```
cd $HOME/mingw/zlib-1.2.8.dfsg
mingw ./configure
mingw make libz.a
make DESTDIR=$HOME/mingw mingw install
```

### Compiling LibreSSL

Tinc can use either OpenSSL or LibreSSL. The latter is recommended.

```
cd $HOME/mingw/libressl-2.3.3
CC=i686-w64-mingw32-gcc ./configure --host=i686-w64-mingw32
make
DESTDIR=$HOME/mingw make install

build ncurses-6.1 
cd ncurses-6.1
CC=i686-w64-mingw32-gcc ./configure --host=i686-w64-mingw32 --without-progs --without-tests --without-debug --with-ticlib --with-termlib

make
make DESTDIR=$HOME/mingw install

Compiling libmyfec

Modify in file 'libmyfec/src/CmakeLists.txt'

add_definitions(--std=gnu99)
set(SOURCE_FILES myfec.c mycommon.c my_debug.c rs.c fec.c sfxhash.c sfhashfcn.c sfmemcap.c sfprimetable.c)

set(CMAKE_C_COMPILER "i686-w64-mingw32-gcc")
set(CMAKE_CXX_COMPILER "i686-w64-mingw32-g++")
#add_library(myfec SHARED ${SOURCE_FILES})
add_library(myfec STATIC ${SOURCE_FILES})

install(
	FILES
		${CMAKE_CURRENT_SOURCE_DIR}/myfec.h
		${CMAKE_CURRENT_SOURCE_DIR}/my_debug.h
		${CMAKE_CURRENT_SOURCE_DIR}/sfxhash.h
		${CMAKE_CURRENT_SOURCE_DIR}/sfhashfcn.h
		${CMAKE_CURRENT_SOURCE_DIR}/sfmemcap.h
		${CMAKE_CURRENT_SOURCE_DIR}/sfprimetable.h
	DESTINATION
		myfec/include
)

install(
	TARGETS myfec
	#LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

cd libmyfec
mkdir build
cd build
cmake ..
make
```

### Compiling tinc

Now that all the dependencies have been cross-compiled, we can cross-compile tinc. Since we use a clone of the git repository here, we need to run `autoreconf` first. If you want to cross-compile tinc from a released tarball, this is not necessary.

```
cd $HOME/mingw/tinc
autoreconf -fsi

./configure --host=i686-w64-mingw32 --with-zlib=$HOME/mingw/usr/local --disable-curses --disable-readline --with-myfec-include=$HOME/mingw/usr/local/include/myfec --with-myfec-lib=$HOME/mingw/usr/local/lib

make
make DESTDIR=$HOME/mingw install
```

### Testing tinc

Since Wine was installed, you can execute the resulting binary even on Linux. Wine does not provide a TAP-Win32 device, but you can use the `DeviceType = dummy` option to test it without. The following command should work in any case:

```
$HOME/mingw/tinc/src/tincd.exe --help
```