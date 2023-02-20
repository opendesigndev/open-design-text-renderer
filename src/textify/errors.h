#pragma once

#include "TextShape.h"
namespace textify {

enum class TextShapeError
{
    OK,             // 0
    NO_PARAGRAPHS,  // 1
    SHAPE_ERROR,    // 2
    TYPESET_ERROR   // 3
};

constexpr const char *errorToString(TextShapeError error) {
    switch (error) {
        case TextShapeError::OK:
            return "OK";
        case TextShapeError::NO_PARAGRAPHS:
            return "NO_PARAGRAPHS";
        case TextShapeError::SHAPE_ERROR:
            return "SHAPE_ERROR";
        case TextShapeError::TYPESET_ERROR:
            return "TYPESET_ERROR";
        default:
            return "???";
    }
}

}
