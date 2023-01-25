
#include "TextShape.h"

#include "textify.h"
#include "PlacedTextData.h"

#include "vendor/fmt/core.h"

#include <cstdlib>
#include <utility>

namespace textify {

TextShape::TextShape(DataPtr&& data) :
    data(std::move(data)) {
}

TextShape::TextShape(PlacedDataPtr&& data) :
    placedData(std::move(data)) {
}

TextShape::TextShape(DataPtr&& data, PlacedDataPtr&& placedData) :
    data(std::move(data)),
    placedData(std::move(placedData)) {
}

void TextShape::deactivate()
{
    active = false;
}

const priv::TextShapeData& TextShape::getData() const
{
    return *data.get();
}

const priv::PlacedTextData& TextShape::getPlacedData() const
{
    return *placedData.get();
}

void TextShape::onFontFaceChanged(const std::string& postScriptName)
{
    if (data->usedFaces.find(postScriptName) != std::end(data->usedFaces)) {
        dirty = true;
    }
}

}
