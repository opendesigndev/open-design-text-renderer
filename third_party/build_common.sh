#!/bin/bash

# sets variables PLATFORM and HOST_PLATFORM
# if EMSCRIPTEN_BUILD build is set the target platform is set Emscripten
set_platform() {
    _OS_NAME=`uname -s`

    case "${_OS_NAME}" in
        Linux)
            HOST_PLATFORM=linux
            ;;
        Darwin)
            HOST_PLATFORM=macOS
            ;;
        MINGW64*|CYGWIN*)
            HOST_PLATFORM=win32_x64
            ;;
        *)
            echo "Unsupported OS: ${OS_NAME}"
            exit 1
            ;;
    esac

    PLATFORM=${HOST_PLATFORM}
    if [ ! -z $EMSCRIPTEN_BUILD ]; then
        PLATFORM=emscripten

        # TODO: test emcc in $PATH
    fi

    export HOST_PLATFORM
    export PLATFORM

    echo Set target platform: ${PLATFORM} and ${HOST_PLATFORM}
}

setup_paths() {
    # $1: absolute project path
    export PROJECT_DIR=${1}
    export SRC_DIR=${PROJECT_DIR}/src
    export PATCHES_DIR=${PROJECT_DIR}/patches
    export BUILD_DIR=${PROJECT_DIR}/build
    export THIRD_PARTY_DIR=`dirname ${PROJECT_DIR}`
    export INSTALL_PREFIX=${THIRD_PARTY_DIR}/platform/${PLATFORM}
}

apply_patches() {
    # sorts patches by the leading number
    for PATCH in `ls -1 "$PATCHES_DIR" | sort -n`; do
        echo Applying -n patch ${PATCH}...
        git apply ${PATCHES_DIR}/${PATCH}
        echo [DONE]
    done
}

fetch() {
    read _REPOSITORY _REVISION < version
    if [[ "$_REPOSITORY" =~ \.git$ ]]; then
        echo Calling fetch for ${_REPOSITORY}@${_REVISION}
        git clone ${_REPOSITORY} src
        cd src
        git checkout ${_REVISION}
        apply_patches
        cd ..
    fi
}

prepare_cmake_build() {
    echo Preparing build...
    mkdir -p ${BUILD_DIR}

    ADDITIONAL_PARAMS=""
    if [ $PLATFORM == "emscripten" ]; then
        ADDITIONAL_PARAMS="-DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake"
    fi

    eval cmake -E chdir ${BUILD_DIR} cmake \
        -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX} \
        -DCMAKE_FIND_ROOT_PATH=${INSTALL_PREFIX} \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
        -DCMAKE_BUILD_TYPE=Release \
        $@ \
        ${ADDITIONAL_PARAMS} \
        ${SRC_DIR}
}

clean() {
    echo "clean | Removing ${BUILD_DIR}..."
    rm -rf ${BUILD_DIR}
}

build_cmake() {
    prepare_cmake_build $@

    VERBOSE=1 cmake --build ${BUILD_DIR} --target install -- -j4
}

CUSTOM_BUILD_SCRIPT_FILE="build.sh"
POST_INSTALL_FILE="post-install.sh"

build_custom() {
    # expects build.sh script in PROJECT_DIR

    echo workdir: $PWD

    ./$CUSTOM_BUILD_SCRIPT_FILE
}

# PREPARE_BUILD_EXTRA additional prepare build arguments
run_build() {
    # $1: project path
    set_platform

    echo Running build process from platform: ${PLATFORM}

    setup_paths ${1}

    echo project dir: $PROJECT_DIR
    echo install prefix: $INSTALL_PREFIX

    cd "${PROJECT_DIR}"

    fetch

    if [[ -f ${CUSTOM_BUILD_SCRIPT_FILE} ]]; then
        echo "Running custom build using build.sh script"
        build_custom ${PREPARE_BUILD_EXTRA}
    else
        echo "Running CMake based"
        build_cmake ${PREPARE_BUILD_EXTRA}
    fi

    if [[ -f ${POST_INSTALL_FILE} ]]; then
        ./${POST_INSTALL_FILE}
    fi
}
