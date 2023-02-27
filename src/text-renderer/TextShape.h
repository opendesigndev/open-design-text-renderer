#pragma once

#include <memory>
#include <string>

#include "TextShapeInput.h"

namespace odtr {

struct PlacedTextData;
namespace priv { struct TextShapeInput; }

struct TextShape
{
    using InputPtr = std::unique_ptr<priv::TextShapeInput>;
    using DataPtr = std::unique_ptr<PlacedTextData>;

    /* implicit */ TextShape(InputPtr &&input, DataPtr &&data);

    /// Input data.
    InputPtr input;
    /// The data.
    DataPtr data;

    bool active = true;
    /// Gets labeled as "dirty" on font face change. Shaping needs to be called again.
    bool dirty = false;

    void deactivate();

    const PlacedTextData& getData() const;

    /// Handle font face change - if a used font changed, mark as dirty.
    void onFontFaceChanged(const std::string &postScriptName);
};

}
