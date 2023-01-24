cmake_minimum_required(VERSION 3.16)
project(textify)

get_directory_property(PARENT_DIR PARENT_DIRECTORY)
if (PARENT_DIR)
    message("textify embedded build")
    set(STANDALONE OFF)
    set(HAVE_LIBOCTOPUS_DEFAULT ON)
else()
    message("textify stadalone build")
    set(STANDALONE ON)
    set(HAVE_LIBOCTOPUS_DEFAULT OFF)
endif()

# TODO: default OFF
option(TEXTIFY_DEBUG "Debug Mode" ON)
option(TEXTIFY_USE_ENCRYPTED_FONTS "Support Avocode fonts encoded via LZ4/Monocypher" ON)
option(TEXTIFY_BUILD_CLI "Build Textify CLI utility" ${STANDALONE})

# TEXTIFY_HAVE_LIBOCTOPUS: if ON LIBOCTOPUS_INCLUDE variable and 'liboctopus' target are expected to be loaded into the current scope
# option(TEXTIFY_HAVE_LIBOCTOPUS "Liboctopus provided externally" $<BOOL:$<NOT,${STANDALONE}>>)
option(TEXTIFY_HAVE_LIBOCTOPUS "Liboctopus provided externally" ${HAVE_LIBOCTOPUS_DEFAULT})
message("have Liboctopus:" ${TEXTIFY_HAVE_LIBOCTOPUS})

if (TEXTIFY_DEBUG)
    add_definitions(-DTEXTIFY_DEBUG)
endif()

# set platform
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()
if (EMSCRIPTEN OR ${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(PLATFORM "emscripten")

    set(CMAKE_CROSSCOMPILING TRUE)
    set(CMAKE_SYSTEM_PROCESSOR x86)

    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

elseif(LINUX)
    set(PLATFORM "linux")
    link_libraries("-ldl")
elseif (WIN32)
    set(PLATFORM "win32_x64")
elseif (APPLE)
    set(PLATFORM "macOS")
endif()

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/platform/${PLATFORM})
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

if (NOT TEXTIFY_HAVE_LIBOCTOPUS)
    add_subdirectory(third_party/liboctopus)
    set(LIBOCTOPUS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/liboctopus)
endif()

find_package(Threads REQUIRED)
find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)
if(NOT TARGET harfbuzz::harfbuzz)
    find_package(harfbuzz REQUIRED)
endif()
find_package(ICU COMPONENTS uc data)

set(TEXTIFY_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(TEXTIFY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (NOT STANDALONE)
    # TODO: might not be needed as this should be set as a part of textify target
    set(TEXTIFY_INCLUDE_DIR ${TEXTIFY_INCLUDE_DIR} PARENT_SCOPE)
endif()

set(TEXTIFY_PUBLIC_HEADERS
    ${TEXTIFY_INCLUDE_DIR}/textify/textify-api.h
    ${TEXTIFY_INCLUDE_DIR}/textify/textify-api-new.h
    ${TEXTIFY_INCLUDE_DIR}/textify/PlacedGlyph.h
    ${TEXTIFY_INCLUDE_DIR}/textify/PlacedDecoration.h
)

set(TEXTIFY_PRIVATE_HEADERS
    ${TEXTIFY_SOURCE_DIR}/common/buffer_view.h
    ${TEXTIFY_SOURCE_DIR}/common/hash_utils.hpp
    ${TEXTIFY_SOURCE_DIR}/common/lexical_cast.hpp
    ${TEXTIFY_SOURCE_DIR}/common/sorted_vector.hpp

    # rendering shim layer
    ${TEXTIFY_SOURCE_DIR}/compat/affine-transform.h
    ${TEXTIFY_SOURCE_DIR}/compat/arithmetics.hpp
    ${TEXTIFY_SOURCE_DIR}/compat/basic-types.h
    ${TEXTIFY_SOURCE_DIR}/compat/bitmap-ops.hpp
    ${TEXTIFY_SOURCE_DIR}/compat/Bitmap.hpp
    ${TEXTIFY_SOURCE_DIR}/compat/pixel-conversion.h
    ${TEXTIFY_SOURCE_DIR}/compat/png.h

    ${TEXTIFY_SOURCE_DIR}/fonts/FaceTable.h
    ${TEXTIFY_SOURCE_DIR}/fonts/FontCodec.h
    ${TEXTIFY_SOURCE_DIR}/fonts/FontManager.h
    ${TEXTIFY_SOURCE_DIR}/fonts/FontStorage.h

    ${TEXTIFY_SOURCE_DIR}/otf/otf.h
    ${TEXTIFY_SOURCE_DIR}/otf/Features.h
    ${TEXTIFY_SOURCE_DIR}/otf/Storage.hpp
    ${TEXTIFY_SOURCE_DIR}/otf/StorageBufferImpl.h
    ${TEXTIFY_SOURCE_DIR}/otf/StorageFileImpl.h

    ${TEXTIFY_SOURCE_DIR}/textify/base.h
    ${TEXTIFY_SOURCE_DIR}/textify/base-types.h
    ${TEXTIFY_SOURCE_DIR}/textify/Config.h
    ${TEXTIFY_SOURCE_DIR}/textify/Context.h
    ${TEXTIFY_SOURCE_DIR}/textify/Face.h
    ${TEXTIFY_SOURCE_DIR}/textify/FreetypeHandle.h
    ${TEXTIFY_SOURCE_DIR}/textify/FormattedParagraph.h
    ${TEXTIFY_SOURCE_DIR}/textify/FormattedText.h
    ${TEXTIFY_SOURCE_DIR}/textify/Glyph.h
    ${TEXTIFY_SOURCE_DIR}/textify/GlyphAcquisitor.h
    ${TEXTIFY_SOURCE_DIR}/textify/GlyphShape.h
    ${TEXTIFY_SOURCE_DIR}/textify/LineBreaker.h
    ${TEXTIFY_SOURCE_DIR}/textify/ParagraphShape.h
    ${TEXTIFY_SOURCE_DIR}/textify/reported-fonts-utils.h
    ${TEXTIFY_SOURCE_DIR}/textify/tabstops.h
    ${TEXTIFY_SOURCE_DIR}/textify/text-format.h
    ${TEXTIFY_SOURCE_DIR}/textify/textify.h
    ${TEXTIFY_SOURCE_DIR}/textify/TextParser.h
    ${TEXTIFY_SOURCE_DIR}/textify/TextShape.h
    ${TEXTIFY_SOURCE_DIR}/textify/TextShapeData.h
    ${TEXTIFY_SOURCE_DIR}/textify/types.h
    ${TEXTIFY_SOURCE_DIR}/textify/TypesetJournal.h
    ${TEXTIFY_SOURCE_DIR}/textify/VisualRun.h
    ${TEXTIFY_SOURCE_DIR}/textify/LineSpan.h
    ${TEXTIFY_SOURCE_DIR}/textify/BitmapWriter.h
    ${TEXTIFY_SOURCE_DIR}/textify/BitmapDraw.h
    ${TEXTIFY_SOURCE_DIR}/textify/PlacedTextData.h

    ${TEXTIFY_SOURCE_DIR}/utils/utils.h
    ${TEXTIFY_SOURCE_DIR}/utils/Log.h
    ${TEXTIFY_SOURCE_DIR}/utils/fmt.h

    ${TEXTIFY_SOURCE_DIR}/unicode/Block.h
    ${TEXTIFY_SOURCE_DIR}/unicode/EmojiTable.h
    ${TEXTIFY_SOURCE_DIR}/unicode/unicode.h

    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/args.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/chrono.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/color.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/compile.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/core.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/format-inl.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/format.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/locale.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/os.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/ostream.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/printf.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/ranges.h
    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/xchar.h
)
set(TEXTIFY_SOURCES
    ${TEXTIFY_SOURCE_DIR}/API/textify-api.cpp
    ${TEXTIFY_SOURCE_DIR}/API/textify-api-new.cpp
    ${TEXTIFY_SOURCE_DIR}/API/PlacedGlyph.cpp
    ${TEXTIFY_SOURCE_DIR}/API/PlacedDecoration.cpp

    ${TEXTIFY_SOURCE_DIR}/common/buffer_view.cpp

    ${TEXTIFY_SOURCE_DIR}/compat/affine-transform.cpp
    ${TEXTIFY_SOURCE_DIR}/compat/arithmetics.cpp
    ${TEXTIFY_SOURCE_DIR}/compat/basic-types.cpp
    ${TEXTIFY_SOURCE_DIR}/compat/bitmap-ops.cpp
    ${TEXTIFY_SOURCE_DIR}/compat/pixel-conversion.cpp
    ${TEXTIFY_SOURCE_DIR}/compat/png.cpp

    ${TEXTIFY_SOURCE_DIR}/fonts/FaceTable.cpp
    ${TEXTIFY_SOURCE_DIR}/fonts/FontCodec.cpp
    ${TEXTIFY_SOURCE_DIR}/fonts/FontManager.cpp
    ${TEXTIFY_SOURCE_DIR}/fonts/FontStorage.cpp

    ${TEXTIFY_SOURCE_DIR}/otf/otf.cpp
    ${TEXTIFY_SOURCE_DIR}/otf/Features.cpp
    ${TEXTIFY_SOURCE_DIR}/otf/StorageBufferImpl.cpp
    ${TEXTIFY_SOURCE_DIR}/otf/StorageFileImpl.cpp

    ${TEXTIFY_SOURCE_DIR}/textify/Config.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/Context.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/Face.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/FreetypeHandle.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/FormattedParagraph.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/FormattedText.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/Glyph.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/GlyphAcquisitor.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/GlyphShape.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/LineBreaker.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/ParagraphShape.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/text-format.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/textify.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/TextParser.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/TextShape.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/TextShapeData.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/TypesetJournal.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/VisualRun.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/LineSpan.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/BitmapWriter.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/BitmapDraw.cpp
    ${TEXTIFY_SOURCE_DIR}/textify/PlacedTextData.cpp

    ${TEXTIFY_SOURCE_DIR}/utils/utils.cpp
    ${TEXTIFY_SOURCE_DIR}/utils/Log.cpp

    ${TEXTIFY_SOURCE_DIR}/unicode/Block.cpp
    ${TEXTIFY_SOURCE_DIR}/unicode/EmojiTable-full.gen.cpp
    ${TEXTIFY_SOURCE_DIR}/unicode/EmojiTable.cpp

    ${TEXTIFY_SOURCE_DIR}/vendor/fmt/format.cc
)

set(TEXTIFY_CLI_HEADERS
    ${TEXTIFY_SOURCE_DIR}/main.cpp
    ${TEXTIFY_SOURCE_DIR}/cli/Canvas.h
    ${TEXTIFY_SOURCE_DIR}/cli/octopus-utils.h
    ${TEXTIFY_SOURCE_DIR}/cli/SimpleRenderer.h
)
set(TEXTIFY_CLI_SOURCES
    ${TEXTIFY_SOURCE_DIR}/cli/Canvas.cpp
    ${TEXTIFY_SOURCE_DIR}/cli/octopus-utils.cpp
    ${TEXTIFY_SOURCE_DIR}/cli/SimpleRenderer.cpp
)

set(TEXTIFY_DEFINITIONS "")

if(TEXTIFY_USE_ENCRYPTED_FONTS)
    set(TEXTIFY_PRIVATE_HEADERS ${TEXTIFY_PRIVATE_HEADERS} ${TEXTIFY_SOURCE_DIR}/vendor/lz4/lz4.h)
    set(TEXTIFY_SOURCES ${TEXTIFY_SOURCES} ${TEXTIFY_SOURCE_DIR}/vendor/lz4/lz4.c)
    set(TEXTIFY_DEFINITIONS ${TEXTIFY_DEFINITIONS} TEXTIFY_USE_LZ4)

    set(TEXTIFY_PRIVATE_HEADERS ${TEXTIFY_PRIVATE_HEADERS} ${TEXTIFY_SOURCE_DIR}/vendor/monocypher/monocypher.h)
    set(TEXTIFY_SOURCES ${TEXTIFY_SOURCES} ${TEXTIFY_SOURCE_DIR}/vendor/monocypher/monocypher.c)
    set(TEXTIFY_DEFINITIONS ${TEXTIFY_DEFINITIONS} TEXTIFY_USE_MONOCYPHER)
endif()


set (TEXTIFY_HEADERS ${TEXTIFY_PUBLIC_HEADERS} ${TEXTIFY_PRIVATE_HEADERS})

# adds static library target
add_library(textify ${TEXTIFY_SOURCES} ${TEXTIFY_HEADERS})

set_property(TARGET textify PROPERTY CXX_STANDARD 17)
target_compile_definitions(textify PRIVATE ${TEXTIFY_DEFINITIONS})

target_include_directories(textify PUBLIC  ${TEXTIFY_INCLUDE_DIR}
                                   PRIVATE ${TEXTIFY_SOURCE_DIR}
                                           ${TEXTIFY_SOURCE_DIR}/vendor
                                           ${ICU_INCLUDE_DIR})

target_link_libraries(textify PRIVATE Threads::Threads PNG::PNG freetype harfbuzz harfbuzz::harfbuzz ${ICU_LIBRARY} liboctopus)

# adds runnable target
if(TEXTIFY_BUILD_CLI AND NOT EMSCRIPTEN)
    add_executable(textify-cli ${TEXTIFY_CLI_SOURCES} ${TEXTIFY_CLI_HEADERS})
    target_include_directories(textify-cli PRIVATE ${TEXTIFY_SOURCE_DIR}/vendor ${TEXTIFY_SOURCE_DIR})
    target_link_libraries(textify-cli PRIVATE liboctopus textify)
endif()
