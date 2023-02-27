#include "FreetypeHandle.h"

namespace odtr {

using namespace compat;

FreetypeHandle::FreetypeHandle() {
    ft_ = nullptr;
}

bool FreetypeHandle::initialize() {
    /*
    Log::instance.logf(Log::TEXTIFY,
                      Log::EXTRA_INFO,
                      "Using Freetype %d.%d.%d and Harfbuzz %s.",
                      FREETYPE_MAJOR,
                      FREETYPE_MINOR,
                      FREETYPE_PATCH,
                      HB_VERSION_STRING);
    */
    error = FT_Init_FreeType(&ft_);
    return checkOk(__func__);
}

void FreetypeHandle::deinitialize() {
    error = FT_Done_FreeType(ft_);
    checkOk(__func__);
}

FreetypeHandle::operator FT_Library() const {
    return ft_;
}

float FreetypeHandle::from16_16fixed(FT_Fixed fixed)
{
    return fixed / 65536.0f;
}

Vector2f FreetypeHandle::from16_16fixed(const FT_Vector& ftVector)
{
    return {from16_16fixed(ftVector.x), from16_16fixed(ftVector.y)};
}

FT_Fixed FreetypeHandle::to16_16fixed(float floating)
{
    return static_cast<FT_Fixed>(floating * 0xffff);
}

FT_Vector FreetypeHandle::to16_16fixed(const Vector2f& vector)
{
    return {to16_16fixed(vector.x), to16_16fixed(vector.y)};
}

float FreetypeHandle::from26_6fixed(FT_F26Dot6 fixed)
{
    return fixed / 64.0f;
}

Vector2f FreetypeHandle::from26_6fixed(const FT_Vector& ftVector)
{
    return {from26_6fixed(ftVector.x), from26_6fixed(ftVector.y)};
}

FT_F26Dot6 FreetypeHandle::to26_6fixed(float floating)
{
    return static_cast<FT_F26Dot6>(floating * 0x40);
}

FT_Vector FreetypeHandle::to26_6fixed(const Vector2f& vector)
{
    return {to26_6fixed(vector.x), to26_6fixed(vector.y)};
}

const char* FreetypeHandle::getErrorMessage(FT_Error err)
{
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  case e: return s;
#define FT_ERROR_START_LIST     switch (err) {
#define FT_ERROR_END_LIST       }
#include FT_ERRORS_H
    return "Unknown Freetype error";
}

bool FreetypeHandle::checkOk(const char* func)
{
    if (error == FT_Err_Ok) {
        return true;
    }

    // FT Error codes: https://freetype.org/freetype2/docs/reference/ft2-error_code_values.html
    // Log::instance.logf(Log::TEXTIFY, Log::ERROR, "Freetype error in function %s: (%d) %s.", func, error, getErrorMessage(error));
    error = FT_Err_Ok;
    return false;
}

FT_Error FreetypeHandle::error = FT_Err_Ok;

} // namespace odtr
