#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace octopus {
    struct Text;
}

namespace textify {

struct Context;
struct TextShape;

struct ContextOptions
{
    using LogFuncType = std::function<void(const std::string&)>;

    LogFuncType errorFunc;
    LogFuncType warnFunc;
    LogFuncType infoFunc;
};

struct Rectangle
{
    int l, t, w, h;
};

struct FRectangle
{
    float l, t, w, h;
};

struct Matrix3f
{
    float m[3][3];
};

struct DrawOptions
{
    /**
     * Text scale factor.
     */
    float scale = 1.0f;

    /**
     * Visible area of the text - unscaled rectangle in the text layer space.
     */
    std::optional<Rectangle> viewArea;
};

struct Dimensions
{
    int width;
    int height;
};

struct DrawTextResult
{
    Rectangle bounds;
    Matrix3f transform;
    bool error;
};

typedef Context* ContextHandle;
typedef TextShape* TextShapeHandle;

/**
 * @brief Creates and initializes Textify's context.
 *
 * A context represent a particular configuration of the engine, it
 * encapsulates a set of fonts and text shapes.
 *
 * Calls on different contexts are thread safe, however, calls using a single
 * context need to be performed either from a single thread or properly
 * synchronized.
 *
 * @param options       context configuration
 *
 * @returns     handle to the created context, can be boolean tested for success
 */
ContextHandle createContext(const ContextOptions& options = {});

/**
 * @brief Removes all the resources attached to the context and destroys the context.
 *
 * All the fonts are removed, all the text shapes destroyed - there is no need
 * to call `destroyTextShapes` explicitly.
 *
 * @param ctx   context handle
 */
void destroyContext(ContextHandle ctx);

/**
 * @brief Loads a single font face from a font or fonts collection stored in a file.
 *
 * Can be called repeatedly with the same @a filename to load multiple faces
 * from a collection. In that case, there's only a single copy of the font data
 * in the internal memory, unless override is set to @a true.
 *
 * Note that @a override causes all the other previously loaded faces to drop.
 * All subsequent shape or draw calls that contain a text depending on one of
 * the dropped faces will fail.
 *
 * @param ctx               context handle
 * @param postScriptName    PostScript name as occurring in `octopus::Text`, required to be non-empty
 * @param inFontFaceName    actual (PostScript) name of a face selected from the font, can be empty, then @a postScriptName is used
 * @param filename          path to a font file
 * @param overwrite          if true, previously loaded data in @a filename is replaced
 */
bool addFontFile(ContextHandle ctx,
                 const std::string& postScriptName,
                 const std::string& inFontFaceName,
                 const std::string& filename,
                 bool overwrite);


/**
 * @brief Loads a single font face from a font or fonts collection stored in memory.
 *
 * In memory version of @see addFontFile. Multiple calls with the same data
 * pointer internally reuse data from the previous call, unless override is set to
 * @a true.
 *
 * Caller is free to release the @a data after the function returns.
 *
 * @param ctx               context handle
 * @param postScriptName    PostScript name as occurring in `octopus::Text`, required to be non-empty
 * @param inFontFaceName    actual (PostScript) name of a face selected from the font, can be empty, then @a postScriptName is used
 * @param data              pointer to font data buffer
 * @param length            length of the @a data buffer
 * @param overwrite          if true, previously loaded data in @a is replaced
 */
bool addFontBytes(ContextHandle ctx,
                  const std::string& postScriptName,
                  const std::string& inFontFaceName,
                  const std::uint8_t* data,
                  size_t length,
                  bool overwrite);

/**
 * @brief For a given Octopus text returns a list of fonts that are used within the text and not yet loaded to the context.
 *
 * @note This function doesn't detect missing glyphs.
 *
 * @param ctx   context handle
 * @param text  Octopus text object
 *
 * @returns list of PostScript names
 */
std::vector<std::string> listMissingFonts(ContextHandle ctx,
                                          const octopus::Text& text);


/**
 * @brief Shapes a given Octopus text and returns a handle to be used to reference the particular text shape within the context.
 *
 * @param ctx   context handle
 * @param text  Octopus text object
 *
 * @returns shaped text handle to be used in draw calls
 */
TextShapeHandle shapeText(ContextHandle ctx,
                          const octopus::Text& text);


/**
 * @brief Destroys multiple text shapes created by @a shapeText call.
 *
 * Removes internal data related to all the listed shapes. All subsequent calls referencing one of the removed shapes will fail.
 *
 * @param ctx        context handle
 * @param textShapes list of shape handles to be destroyed
 * @param count      number of shapes
 */
void destroyTextShapes(ContextHandle ctx,
                       TextShapeHandle* textShapes,
                       size_t count);


/**
 * @brief Updates an existing text shape with a newly provided text format.
 *
 * @param ctx         context handle
 * @param textShape   existing text shape handle
 * @param text        new Octopus text object
 *
 * @returns     boolean value indicating success of the call
 */
bool reshapeText(ContextHandle ctx,
                 TextShapeHandle textShape,
                 const octopus::Text& text);


/**
 * @brief For a given text shape returns its "logical bounds" (i.e. a frame that contains the text) with text transformation applied.
 *
 * @note Text frame doesn't always have integer dimensions, hence return value type is FRectangle.
 *
 * @param ctx         context handle
 * @param textShape   text shape handle
 *
 * @returns     transformed logical bounds
 */
FRectangle getBounds(ContextHandle ctx,
                     TextShapeHandle textShape);


/**
 * @brief Detects whether a point is inside of the given text.
 *
 * @note    Implemented only a coarse test testing the point being inside the text bounding box.
 *
 * @param ctx         context handle
 * @param textShape   text shape handle
 * @param x           point's x-coordinate
 * @param y           point's y-coordinate
 * @param radius      maximal distance of the point to any portion of the text for the test to successfully pass
 *
 * @returns     true if a point is inside of the given text
 **/
bool intersect(ContextHandle ctx,
               TextShapeHandle textShape,
               float x,
               float y,
               float radius);


/**
 * @brief Computes dimensions of a raster that contains all the text drawn with a certain @a drawOptions.
 *
 * The result is used as an input to @a drawText.
 *
 * @param ctx         context handle
 * @param textShape   text shape handle
 * @param drawOptions draw configuration
 *
 * @returns     width and height of the buffer
 **/
Dimensions getDrawBufferDimensions(ContextHandle ctx,
                                   TextShapeHandle textShape,
                                   const DrawOptions& drawOptions = {});


/**
 * @brief Draws a text shape into a buffer.
 *
 * Caller is expected to allocate the buffer, pixel size is 4 bytes.  Output
 * pixel format is RGBA with red channel being stored in the least significant
 * byte.
 *
 * @a drawOptions allows to set text scale and view area (cutout)
 *
 * @param ctx         context handle
 * @param textShape   text shape handle
 * @param pixels      pointer to the start of a buffer to draw into
 * @param width       number of columns in the buffer
 * @param height      number of rows in the buffer
 * @param drawOptions draw configuration
 *
 * @returns   result of the draw call
 **/
DrawTextResult drawText(ContextHandle ctx,
                        TextShapeHandle textShape,
                        void* pixels, int width, int height,
                        const DrawOptions& drawOptions = {});

/// Shape text.
// TODO: Matus: New textify functionality. Cleanup.
TextShapeHandle shapePlacedText(ContextHandle ctx,
                                const octopus::Text& text);

/// Draw text.
// TODO: Matus: New textify functionality. Cleanup.
DrawTextResult drawPlacedText(ContextHandle ctx,
                              TextShapeHandle textShape,
                              void* pixels, int width, int height,
                              const DrawOptions& drawOptions = {});

}
