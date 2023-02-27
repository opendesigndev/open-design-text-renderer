cmake_minimum_required(VERSION 3.16)
project(open-design-text-renderer)

get_directory_property(PARENT_DIR PARENT_DIRECTORY)
if (PARENT_DIR)
    message("open-design-text-renderer embedded build")
    set(STANDALONE OFF)
    set(HAVE_LIBOCTOPUS_DEFAULT ON)
else()
    message("open-design-text-renderer standalone build")
    set(STANDALONE ON)
    set(HAVE_LIBOCTOPUS_DEFAULT OFF)
endif()

# TODO: default OFF
option(TEXT_RENDERER_DEBUG "Debug Mode" ON)
option(TEXT_RENDERER_USE_ENCRYPTED_FONTS "Support Avocode fonts encoded via LZ4/Monocypher" ON)
option(TEXT_RENDERER_BUILD_CLI "Build OD Text Renderer CLI utility" ${STANDALONE})

# TEXT_RENDERER_HAVE_LIBOCTOPUS: if ON LIBOCTOPUS_INCLUDE variable and 'liboctopus' target are expected to be loaded into the current scope
# option(TEXT_RENDERER_HAVE_LIBOCTOPUS "Liboctopus provided externally" $<BOOL:$<NOT,${STANDALONE}>>)
option(TEXT_RENDERER_HAVE_LIBOCTOPUS "Liboctopus provided externally" ${HAVE_LIBOCTOPUS_DEFAULT})
message("have Liboctopus:" ${TEXT_RENDERER_HAVE_LIBOCTOPUS})

if (TEXT_RENDERER_DEBUG)
    add_definitions(-DTEXT_RENDERER_DEBUG)
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

if (NOT TEXT_RENDERER_HAVE_LIBOCTOPUS)
    add_subdirectory(third_party/liboctopus)
    set(LIBOCTOPUS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/liboctopus)
endif()

# Add test project if specified
if(ODE_TEST_MODULES)
    add_subdirectory(test)
endif()

find_package(Threads REQUIRED)
find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)
if(NOT TARGET harfbuzz::harfbuzz)
    find_package(harfbuzz REQUIRED)
endif()
find_package(ICU COMPONENTS uc data)

set(TEXT_RENDERER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(TEXT_RENDERER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (NOT STANDALONE)
    # TODO: might not be needed as this should be set as a part of Text Renderer target
    set(TEXT_RENDERER_INCLUDE_DIR ${TEXT_RENDERER_INCLUDE_DIR} PARENT_SCOPE)
endif()

set(TEXT_RENDERER_PUBLIC_HEADERS
    ${TEXT_RENDERER_INCLUDE_DIR}/open-design-text-renderer/text-renderer-api.h
    ${TEXT_RENDERER_INCLUDE_DIR}/open-design-text-renderer/PlacedGlyph.h
    ${TEXT_RENDERER_INCLUDE_DIR}/open-design-text-renderer/PlacedDecoration.h
    ${TEXT_RENDERER_INCLUDE_DIR}/open-design-text-renderer/PlacedTextData.h
)

set(TEXT_RENDERER_PRIVATE_HEADERS
    ${TEXT_RENDERER_SOURCE_DIR}/common/buffer_view.h
    ${TEXT_RENDERER_SOURCE_DIR}/common/hash_utils.hpp
    ${TEXT_RENDERER_SOURCE_DIR}/common/lexical_cast.hpp
    ${TEXT_RENDERER_SOURCE_DIR}/common/sorted_vector.hpp

    # rendering shim layer
    ${TEXT_RENDERER_SOURCE_DIR}/compat/affine-transform.h
    ${TEXT_RENDERER_SOURCE_DIR}/compat/arithmetics.hpp
    ${TEXT_RENDERER_SOURCE_DIR}/compat/basic-types.h
    ${TEXT_RENDERER_SOURCE_DIR}/compat/bitmap-ops.hpp
    ${TEXT_RENDERER_SOURCE_DIR}/compat/Bitmap.hpp
    ${TEXT_RENDERER_SOURCE_DIR}/compat/pixel-conversion.h
    ${TEXT_RENDERER_SOURCE_DIR}/compat/png.h

    ${TEXT_RENDERER_SOURCE_DIR}/fonts/FaceTable.h
    ${TEXT_RENDERER_SOURCE_DIR}/fonts/FontCodec.h
    ${TEXT_RENDERER_SOURCE_DIR}/fonts/FontManager.h
    ${TEXT_RENDERER_SOURCE_DIR}/fonts/FontStorage.h

    ${TEXT_RENDERER_SOURCE_DIR}/otf/otf.h
    ${TEXT_RENDERER_SOURCE_DIR}/otf/Features.h
    ${TEXT_RENDERER_SOURCE_DIR}/otf/Storage.hpp
    ${TEXT_RENDERER_SOURCE_DIR}/otf/StorageBufferImpl.h
    ${TEXT_RENDERER_SOURCE_DIR}/otf/StorageFileImpl.h

    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/base.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/base-types.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/Config.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/Context.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/Face.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/FreetypeHandle.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/FormattedParagraph.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/FormattedText.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/Glyph.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/GlyphAcquisitor.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/GlyphShape.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/LineBreaker.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/ParagraphShape.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/reported-fonts-utils.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/tabstops.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/text-format.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/text-renderer.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TextParser.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TextShape.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TextShapeInput.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TextShapeData.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/types.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TypesetJournal.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/VisualRun.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/LineSpan.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/BitmapWriter.h
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/PlacedTextRendering.h

    ${TEXT_RENDERER_SOURCE_DIR}/utils/utils.h
    ${TEXT_RENDERER_SOURCE_DIR}/utils/Log.h
    ${TEXT_RENDERER_SOURCE_DIR}/utils/fmt.h

    ${TEXT_RENDERER_SOURCE_DIR}/unicode/Block.h
    ${TEXT_RENDERER_SOURCE_DIR}/unicode/EmojiTable.h
    ${TEXT_RENDERER_SOURCE_DIR}/unicode/unicode.h

    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/args.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/chrono.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/color.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/compile.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/core.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/format-inl.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/format.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/locale.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/os.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/ostream.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/printf.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/ranges.h
    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/xchar.h
)
set(TEXT_RENDERER_SOURCES
    ${TEXT_RENDERER_SOURCE_DIR}/api/text-renderer-api.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/api/PlacedGlyph.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/api/PlacedDecoration.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/api/PlacedTextData.cpp

    ${TEXT_RENDERER_SOURCE_DIR}/common/buffer_view.cpp

    ${TEXT_RENDERER_SOURCE_DIR}/compat/affine-transform.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/compat/arithmetics.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/compat/basic-types.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/compat/bitmap-ops.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/compat/pixel-conversion.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/compat/png.cpp

    ${TEXT_RENDERER_SOURCE_DIR}/fonts/FaceTable.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/fonts/FontCodec.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/fonts/FontManager.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/fonts/FontStorage.cpp

    ${TEXT_RENDERER_SOURCE_DIR}/otf/otf.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/otf/Features.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/otf/StorageBufferImpl.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/otf/StorageFileImpl.cpp

    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/Config.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/Context.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/Face.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/FreetypeHandle.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/FormattedParagraph.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/FormattedText.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/Glyph.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/GlyphAcquisitor.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/GlyphShape.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/LineBreaker.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/ParagraphShape.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/text-format.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/text-renderer.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TextParser.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TextShape.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TextShapeInput.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TextShapeData.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/TypesetJournal.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/VisualRun.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/LineSpan.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/BitmapWriter.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/text-renderer/PlacedTextRendering.cpp

    ${TEXT_RENDERER_SOURCE_DIR}/utils/utils.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/utils/Log.cpp

    ${TEXT_RENDERER_SOURCE_DIR}/unicode/Block.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/unicode/EmojiTable-full.gen.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/unicode/EmojiTable.cpp

    ${TEXT_RENDERER_SOURCE_DIR}/vendor/fmt/format.cc
)

set(TEXT_RENDERER_CLI_HEADERS
    ${TEXT_RENDERER_SOURCE_DIR}/cli/Canvas.h
    ${TEXT_RENDERER_SOURCE_DIR}/cli/octopus-utils.h
    ${TEXT_RENDERER_SOURCE_DIR}/cli/SimpleRenderer.h
)
set(TEXT_RENDERER_CLI_SOURCES
    ${TEXT_RENDERER_SOURCE_DIR}/main.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/cli/Canvas.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/cli/octopus-utils.cpp
    ${TEXT_RENDERER_SOURCE_DIR}/cli/SimpleRenderer.cpp
)

set(TEXT_RENDERER_DEFINITIONS "")

if(TEXT_RENDERER_USE_ENCRYPTED_FONTS)
    set(TEXT_RENDERER_PRIVATE_HEADERS ${TEXT_RENDERER_PRIVATE_HEADERS} ${TEXT_RENDERER_SOURCE_DIR}/vendor/lz4/lz4.h)
    set(TEXT_RENDERER_SOURCES ${TEXT_RENDERER_SOURCES} ${TEXT_RENDERER_SOURCE_DIR}/vendor/lz4/lz4.c)
    set(TEXT_RENDERER_DEFINITIONS ${TEXT_RENDERER_DEFINITIONS} TEXT_RENDERER_USE_LZ4)

    set(TEXT_RENDERER_PRIVATE_HEADERS ${TEXT_RENDERER_PRIVATE_HEADERS} ${TEXT_RENDERER_SOURCE_DIR}/vendor/monocypher/monocypher.h)
    set(TEXT_RENDERER_SOURCES ${TEXT_RENDERER_SOURCES} ${TEXT_RENDERER_SOURCE_DIR}/vendor/monocypher/monocypher.c)
    set(TEXT_RENDERER_DEFINITIONS ${TEXT_RENDERER_DEFINITIONS} TEXT_RENDERER_USE_MONOCYPHER)
endif()


set (TEXT_RENDERER_HEADERS ${TEXT_RENDERER_PUBLIC_HEADERS} ${TEXT_RENDERER_PRIVATE_HEADERS})

# adds static library target
add_library(${PROJECT_NAME} ${TEXT_RENDERER_SOURCES} ${TEXT_RENDERER_HEADERS})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${TEXT_RENDERER_DEFINITIONS})

target_include_directories(${PROJECT_NAME} PUBLIC  ${TEXT_RENDERER_INCLUDE_DIR}
                                           PRIVATE ${TEXT_RENDERER_SOURCE_DIR}
                                                   ${TEXT_RENDERER_SOURCE_DIR}/vendor
                                                   ${ICU_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads PNG::PNG freetype harfbuzz harfbuzz::harfbuzz ${ICU_LIBRARY} liboctopus)

# adds runnable target
if(TEXT_RENDERER_BUILD_CLI AND NOT EMSCRIPTEN)
    add_executable(open-design-text-renderer-cli ${TEXT_RENDERER_CLI_SOURCES} ${TEXT_RENDERER_CLI_HEADERS})
    target_include_directories(open-design-text-renderer-cli PRIVATE ${TEXT_RENDERER_SOURCE_DIR}/vendor ${TEXT_RENDERER_SOURCE_DIR})
    target_link_libraries(open-design-text-renderer-cli PRIVATE liboctopus open-design-text-renderer)
endif()
