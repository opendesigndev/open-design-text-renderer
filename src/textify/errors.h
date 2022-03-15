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

}
