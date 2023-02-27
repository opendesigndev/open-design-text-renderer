
#include "TextShape.h"

#include <open-design-text-renderer/PlacedTextData.h>

#include "text-renderer.h"
#include "TextShapeInput.h"

namespace odtr {

TextShape::TextShape(InputPtr &&input, DataPtr &&data) :
    input(std::move(input)),
    data(std::move(data)) {
}

void TextShape::deactivate()
{
    active = false;
}

const PlacedTextData& TextShape::getData() const
{
    return *data.get();
}

void TextShape::onFontFaceChanged(const std::string &postScriptName)
{
    if (input->usedFaces.find(postScriptName) != std::end(input->usedFaces)) {
        dirty = true;
    }
}

}
