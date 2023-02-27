
#include "TextShape.h"

#include <textify/PlacedTextData.h>

#include "textify.h"
#include "TextShapeInput.h"

namespace textify {

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
