#!/usr/bin/env bash
VERSION="64_2"

OS_NAME=`uname -s`

case "$OS_NAME" in
    Linux)
        HOST_PLATFORM=linux
        HOST_PREFIX="--host=x86_64-linux-gnu"
        ;;
    Darwin)
        HOST_PLATFORM=macOS
        HOST_PREFIX="--host=x86_64-apple-darwin"
        ;;
    MINGW64*|CYGWIN*)
        HOST_PLATFORM=win32_x64
        ;;
    *)
        echo "Unsupported OS: ${OS_NAME}"
        exit 1
        ;;
esac

get_abs_filename() {
  # $1 : relative filename
  if [ -d "$(dirname "$1")" ]; then
    echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
  fi
}

setup_toolchain() {
    _PLATFORM=$1
    echo Setting up toolchain for: $_PLATFORM
    if [ $_PLATFORM == "linux" ]; then
        CC=gcc
        CXX=g++
        MAKE=make
        CONFIGURE=./configure
    elif [ $_PLATFORM == "macOS" ]; then
        CC=/usr/local/opt/llvm/bin/clang
        CXX=/usr/local/opt/llvm/bin/clang++
        MAKE=gmake
        CONFIGURE=./configure
    elif [ $_PLATFORM == "emscripten" ]; then
        setup_toolchain $HOST_PLATFORM

        # MAKE="emmake $MAKE"
        MAKE="emmake make"
        CONFIGURE="emconfigure ./configure"
    fi

    echo "Toolchain:"
    echo -e "\tCC = $CC"
    echo -e "\tCXX = $CXX"
    echo -e "\tMAKE = $MAKE"
    echo -e "\tCONFIGURE = $CONFIGURE"
}

run_configure() {
    # export CFLAGS=-pthread
    # export CXXFLAGS=-pthread
    $CONFIGURE --disable-extras --disable-icuio --disable-tests --disable-samples --disable-shared --enable-static --prefix=$INSTALL_PREFIX $@
}

run_install() {
    $MAKE install
}

PLATFORM=${HOST_PLATFORM}
if [ ! -z $EMSCRIPTEN_BUILD ]; then
    PLATFORM=emscripten

    # TODO: test emcc in $PATH
fi

echo Building for platform: $PLATFORM

ROOT_DIR="`dirname ${0}`"
ROOT_DIR=`get_abs_filename $ROOT_DIR`

THIRD_PARTY_DIR=${ROOT_DIR}/..
INSTALL_PREFIX=${THIRD_PARTY_DIR}/platform/${PLATFORM}/

SRC_DIR=$ROOT_DIR/icu

echo "Unpacking sources..."

tar xzf $ROOT_DIR/icu4c-${VERSION}-src.tar.gz -C $ROOT_DIR

echo "Done."

mkdir -p $INSTALL_PREFIX

echo "Root dir": $ROOT_DIR
echo "Working directory (icu source root)": $SRC_DIR
echo "Install prefix": $INSTALL_PREFIX

if [ ! -f $SRC_DIR/icu4c/source/configure ]; then
    echo "Missing configure file, likely repository isn't cloned, try updating git submodules"
    exit 1
fi

echo Patching sources...

patch $SRC_DIR/icu4c/source/data/Makefile.in $SRC_DIR/../patches/Makefile.data.patch
patch $SRC_DIR/icu4c/source/extra/uconv/Makefile.in $SRC_DIR/../patches/Makefile.extra.uconv.patch

pushd $SRC_DIR/icu4c/source

echo Jumped to source dir: `pwd`

setup_toolchain ${HOST_PLATFORM}

export ICU_DATA_FILTER_FILE=$ROOT_DIR/data_filter.json

echo Running ./configure...

$MAKE distclean

run_configure ${HOST_PREFIX}

echo Running make

$MAKE -j8

if [ $PLATFORM != "emscripten" ]; then
    # install to prefix
    run_install
else
    rm -r bin.bak
    mv bin bin.bak

    $MAKE distclean

    # setup emscripten toolchain
    setup_toolchain $PLATFORM

    run_configure --host=i686-pc-linux

    # run make for the first time and let it fail
    $MAKE -j8

    rm -r bin
    cp -r bin.bak bin

    # run make for the second time
    $MAKE

    run_install
fi
