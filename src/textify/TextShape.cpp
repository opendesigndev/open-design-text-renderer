#include "TextShape.h"
#include "textify.h"

#include "vendor/fmt/core.h"

#include <cstdlib>
#include <utility>

namespace textify {

TextShape::TextShape(DataPtr&& data)
    : data(std::move(data)) {
}

void TextShape::deactivate()
{
    active = false;
}

const priv::TextShapeData& TextShape::getData() const
{
    return *data.get();
}

void TextShape::onFontFaceChanged(const std::string& postScriptName)
{
    if (data->usedFaces.find(postScriptName) != std::end(data->usedFaces)) {
        dirty = true;
    }
}

}
