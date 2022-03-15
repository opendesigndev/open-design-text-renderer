#include "Face.h"
#include "otf/otf.h"
// REFACTOR
// #include "logging/BasicLogger.h"
#include <limits>
#include <string>
#include <cassert>

namespace textify {

using namespace compat;

Face::Face(FT_Library ftLibrary, const char* filename, FT_Long faceIndex) : hbFont_(nullptr)
{
    FreetypeHandle::error = FT_New_Face(ftLibrary, filename, faceIndex, &ftFace_);
    if (FreetypeHandle::checkOk(__func__)) {
        initialize();
        if (auto featuresResult = otf::listFeatures(filename)) {
            features_ = featuresResult.moveValue();
        }
    } else {
        ftFace_ = nullptr;
    }
}

Face::Face(FT_Library ftLibrary, const byte* fileBytes, int length, FT_Long faceIndex) : hbFont_(nullptr)
{
    FreetypeHandle::error = FT_New_Memory_Face(ftLibrary, fileBytes, length, faceIndex, &ftFace_);
    if (FreetypeHandle::checkOk(__func__)) {
        initialize();
        if (auto featuresResult = otf::listFeatures(fileBytes, length)) {
            features_ = featuresResult.moveValue();
        }
    } else {
        ftFace_ = nullptr;
    }
}

void Face::initialize()
{
    recreateHBFont();

    const char* psname = FT_Get_Postscript_Name(ftFace_);
    postscriptName_ = psname ? std::string(psname) : "";

    params_.isColor = isColorFont();
    params_.scalable = FT_IS_SCALABLE(ftFace_);
    params_.loadflags = params_.isColor ? FT_LOAD_COLOR : FT_LOAD_DEFAULT;
    FreetypeHandle::error = FT_Err_Ok;
}

void Face::recreateHBFont()
{
    if (hbFont_) {
        hb_font_destroy(hbFont_);
    }

    hbFont_ = hb_ft_font_create_referenced(ftFace_);
}

Face::~Face()
{
    hb_font_destroy(hbFont_);
    FreetypeHandle::error = FT_Done_Face(ftFace_);
    FreetypeHandle::checkOk(__func__);
}

bool Face::ready() const
{
    return ftFace_ != nullptr && hbFont_ != nullptr;
}

void Face::logFeatures(bool all) const
{
    std::unordered_map<std::string, bool> features;

    // macros return values of the tags
    features["FT_HAS_HORIZONTAL      "] = 0 != (FT_HAS_HORIZONTAL(ftFace_));
    features["FT_HAS_VERTICAL        "] = 0 != (FT_HAS_VERTICAL(ftFace_));
    features["FT_HAS_KERNING         "] = 0 != (FT_HAS_KERNING(ftFace_));
    features["FT_HAS_FIXED_SIZES     "] = 0 != (FT_HAS_FIXED_SIZES(ftFace_));
    features["FT_HAS_GLYPH_NAMES     "] = 0 != (FT_HAS_GLYPH_NAMES(ftFace_));
    features["FT_HAS_COLOR           "] = 0 != (FT_HAS_COLOR(ftFace_));
    features["FT_HAS_MULTIPLE_MASTERS"] = 0 != (FT_HAS_MULTIPLE_MASTERS(ftFace_));
    features["FT_IS_SFNT             "] = 0 != (FT_IS_SFNT(ftFace_));
    features["FT_IS_SCALABLE         "] = 0 != (FT_IS_SCALABLE(ftFace_));
    features["FT_IS_FIXED_WIDTH      "] = 0 != (FT_IS_FIXED_WIDTH(ftFace_));
    features["FT_IS_CID_KEYED        "] = 0 != (FT_IS_CID_KEYED(ftFace_));
    features["FT_IS_TRICKY           "] = 0 != (FT_IS_TRICKY(ftFace_));
#ifndef __EMSCRIPTEN__
    features["FT_IS_NAMED_INSTANCE   "] = 0 != (FT_IS_NAMED_INSTANCE(ftFace_));
#endif
    // features["FT_IS_VARIATION        "] = 0!=(FT_IS_VARIATION(ftFace_)); // not supported until FreeType 2.9

    // REFACTOR
    /*
    Log::instance.logf(
        Log::TEXTIFY, Log::EXTRA_INFO, "Available features for %s %s:", ftFace_->family_name, ftFace_->style_name);
    for (const auto& f : features) {
        if (all) {
            Log::instance.logf(Log::TEXTIFY, Log::EXTRA_INFO, "    %s\t%d", f.first.c_str(), f.second);
        } else if (f.second) {
            Log::instance.logf(Log::TEXTIFY, Log::EXTRA_INFO, "    %s", f.first.c_str());
        }
    }
    */
    isColorFont(true);
}

bool Face::isColorFont(bool log) const
{
    static const FT_Tag microsoft = FT_MAKE_TAG('C', 'O', 'L', 'R');
    static const FT_Tag apple     = FT_MAKE_TAG('s', 'b', 'i', 'x');
    static const FT_Tag google    = FT_MAKE_TAG('C', 'B', 'D', 'T');
    static const FT_Tag adobe     = FT_MAKE_TAG('S', 'V', 'G', ' ');

    auto checkTag = [&](const FT_Tag& tag, char const* style, char const* addendum) -> bool {
        unsigned long length = 0;
        FT_Load_Sfnt_Table(ftFace_, tag, 0, nullptr, &length);
        if (length) {
            if (log) {
                // REFACTOR
                /*
                Log::instance.logf(Log::TEXTIFY,
                                   Log::EXTRA_INFO,
                                   "%s %s is %s style color font, which %s",
                                   ftFace_->family_name,
                                   ftFace_->style_name,
                                   style,
                                   addendum);
                */
            }
            return true;
        }
        return false;
    };

    auto colr = checkTag(microsoft, "Microsoft", "uses color palletes");
    auto cbdt = checkTag(google, "Google",
                         "seems to work OK, but can hardly appear in a design as neither Apple or Win support it.");
    params_.cpalUsed = colr;

    auto isColor =
        checkTag(apple, "Apple", "works OK.") ||
        checkTag(google,
                 "Google",
                 "seems to work OK, but can hardly appear in a design as neither Apple or Win support it.") ||
        checkTag(adobe, "Adobe", "is not yet supported by FreeType.") || cbdt ||
    params_.cpalUsed;

    return isColor;
}

hb_font_t* Face::getHbFont() const
{
    return hbFont_;
}

Result<font_size,bool> Face::setSize(font_size size)
{
    auto selectedSize = size;
    auto charSize = FreetypeHandle::to26_6fixed(float(size));
    if (params_.scalable) {
        FreetypeHandle::error = FT_Set_Char_Size(ftFace_, 0, charSize, 0, 0);
    } else {
        if (ftFace_->num_fixed_sizes == 0)
            return false;

        int best_match = 0;
        int diff = std::numeric_limits<int>::max();
        for (int i = 0; i < ftFace_->num_fixed_sizes; ++i) {
            int ndiff = std::abs(charSize - ftFace_->available_sizes[i].size);
            if (ndiff < diff) {
                best_match = i;
                diff = ndiff;
            }
        }
        selectedSize = font_size(FreetypeHandle::from26_6fixed(ftFace_->available_sizes[best_match].size));
        FreetypeHandle::error = FT_Select_Size(ftFace_, best_match);
    }

    // need to create new hb_font after resizing the FT_Face
    recreateHBFont();

    if (!FreetypeHandle::checkOk(__func__)) {
        return false;
    }

    return selectedSize;
}

const std::string& Face::getPostScriptName() const
{
    return postscriptName_;
}

GlyphPtr Face::acquireGlyph(FT_UInt codepoint, const Vector2f& offset, const ScaleParams& scale, bool render, bool disableHinting) const
{
    auto params = params_;
    if (disableHinting) {
        params.loadflags |= FT_LOAD_NO_HINTING /*| FT_LOAD_NO_AUTOHINT*/;
    }
    acquisitor_.setup(params, ftFace_);
    return acquisitor_.acquire(codepoint, offset, scale, render);
}

FT_Fixed Face::getGlyphAdvance(hb_codepoint_t codepoint) const
{
    FT_Fixed advance;
    FreetypeHandle::error = FT_Get_Advance(ftFace_, codepoint, params_.loadflags, &advance);
    FreetypeHandle::checkOk(__func__);
    return advance;
}

bool Face::hasGlyph(qchar cp) const
{
    auto idx = FT_Get_Char_Index(ftFace_, cp);
    return idx != 0;
}

bool Face::hasOpenTypeFeature(const std::string& featureTag) const
{
    return features_.hasFeature(featureTag);
}

float Face::scaleFontUnits(int fontParam, bool y_scale) const
{
    return FreetypeHandle::from26_6fixed(
        (FT_MulFix(fontParam, y_scale ? ftFace_->size->metrics.y_scale : ftFace_->size->metrics.x_scale)));
}

FacePtr::FacePtr() : face_(nullptr) {}

FacePtr::FacePtr(Face* face) : face_(face) {}

void FacePtr::destroy()
{
    delete face_;
}

Face* FacePtr::operator->() const
{
    return face_;
}

FacePtr::operator Face*() const
{
    return face_;
}

} // namespace textify
