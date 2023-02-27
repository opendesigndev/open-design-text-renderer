#pragma once

#include "base.h"

namespace odtr {

/**
 * This is a wrapper for the FT_Library handle, representing an instance of FreeType, which can be used to create Faces.
 * Call initialize and check its return value. Call deinitialize when done.
 */
class FreetypeHandle {

public:
    FreetypeHandle();
    bool initialize();
    void deinitialize();
    operator FT_Library() const;

    static float    from16_16fixed(FT_Fixed fixed);            ///< Convert 16.16 fixed point font units to pixels
    static compat::Vector2f from16_16fixed(const FT_Vector& ftVector); ///< Convert 16.16 fixed point font units vector
    static FT_Fixed   to16_16fixed(float floating);            ///< Convert pixels to 16.16 fixed point font units
    static FT_Vector  to16_16fixed(const compat::Vector2f& vector);    ///< Convert pixels to 16.16 fixed point font units vector

    static float    from26_6fixed(FT_F26Dot6 fixed);           ///< Convert 26.6 fixed point 1/64 fractional pixels to pixels
    static compat::Vector2f from26_6fixed(const FT_Vector& ftVector);  ///< Convert 26.6 fixed point 1/64 fractional pixels vector
    static FT_F26Dot6 to26_6fixed(float floating);             ///< Convert pixels to 26.6 fixed point 1/64 fractional pixels
    static FT_Vector  to26_6fixed(const compat::Vector2f& vector);     ///< Convert pixels to 26.6 fixed point 1/64 fractional vector

    static FT_F26Dot6 round26_6(FT_F26Dot6 x) { return (x + 32) & -64; } ///< See FreeType outlines tutorial
    static FT_F26Dot6 floor26_6(FT_F26Dot6 x) { return        x & -64; } ///< See FreeType outlines tutorial
    static FT_F26Dot6 ceil26_6 (FT_F26Dot6 x) { return (x + 63) & -64; } ///< See FreeType outlines tutorial

    static const char* getErrorMessage(FT_Error err);
    static bool checkOk(const char* func);

    static FT_Error error;

private:
    FT_Library ft_;

};

} // namespace odtr
