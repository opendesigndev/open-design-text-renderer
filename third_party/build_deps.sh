#!/bin/bash

## env variables
# - HEADLESS_GL      - sets osmesa as a GL provider
# - MULTI_GL_BUILD   - allows multiple GL implementations to coexist in the install dir
#                      (selected via ./link_gl.sh script)

# returns absolute filename for given relative file
get_abs_filename() {
  # $1 : relative filename
  filename=$1
  parentdir=$(dirname "${filename}")

  if [ -d "${filename}" ]; then
      echo "$(cd "${filename}" && pwd)"
  elif [ -d "${parentdir}" ]; then
    echo "$(cd "${parentdir}" && pwd)/$(basename "${filename}")"
  fi
}

SCRIPT_DIR=`dirname ${0}`

DEPS_BASE_DIR=`get_abs_filename ${SCRIPT_DIR}`

source ${DEPS_BASE_DIR}/build_common.sh

LIBZ_DIR=${DEPS_BASE_DIR}/zlib
LIBPNG_DIR=${DEPS_BASE_DIR}/libpng
FREETYPE_DIR=${DEPS_BASE_DIR}/freetype
HARFBUZZ_DIR=${DEPS_BASE_DIR}/harfbuzz
ICU_DIR=${DEPS_BASE_DIR}/icu
SKIA_DIR=${DEPS_BASE_DIR}/skia

FREETYPE_PREPARE_EXTRA_COMMON="-DCMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE"

GL_PROVIDER="glx"

if [[ $HEADLESS_GL == 1 ]]; then
    GL_PROVIDER="osmesa"
fi
export GL_PROVIDER

ALL_TARGETS="icu libz libpng freetype harfbuzz"

build_libs() {

    TARGETS=${*}

    if [[ -z ${TARGETS} ]]; then
        TARGETS=${ALL_TARGETS}
    fi

    echo building for: ${TARGETS}

    # note: Skia and ICU use custom build scripts

    for target in ${TARGETS}; do
        case ${target} in
            icu)
                echo Building ICU...
                ${ICU_DIR}/build.sh
                ;;

            libz)
                echo Building libz...
                run_build ${LIBZ_DIR}
                clean
                ;;

            libpng)
                echo Building libpng...
                PREPARE_BUILD_EXTRA="-DPNG_SHARED=OFF -DPNG_TESTS=OFF -DPNG_HARDWARE_OPTIMIZATIONS=OFF"
                run_build ${LIBPNG_DIR}
                clean
                ;;

            freetype)
                echo Building Freetype without HarfBuzz...
                PREPARE_BUILD_EXTRA="${FREETYPE_PREPARE_EXTRA_COMMON} -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=TRUE"
                run_build ${FREETYPE_DIR}
                clean
                ;;

            harfbuzz)
                echo Building HarfBuzz...
                PREPARE_BUILD_EXTRA="-DHB_BUILD_TESTS=OFF -DHB_HAVE_FREETYPE=ON -DHB_HAVE_ICU=OFF -DHB_HAVE_CORETEXT=OFF"
                run_build ${DEPS_BASE_DIR}/harfbuzz
                clean
                ;;

            *)
                echo Unknown target: ${target}
                ;;

        esac
    done

    if [[ ${TARGETS} == *"freetype"* ]] && [[ ${TARGETS} == *"harfbuzz"* ]]; then
        # Freetype with Harfbuzz
        echo Building Freetype with HarfBuzz...

        PREPARE_BUILD_EXTRA=${FREETYPE_PREPARE_EXTRA_COMMON}
        run_build ${FREETYPE_DIR}
        clean
    fi
}

build_libs ${*}
