#pragma once

#include <memory>
#include <string>

namespace textify {

namespace priv {
struct TextShapeData;
}
struct PlacedTextData;

struct TextShape
{
    using DataPtr = std::unique_ptr<priv::TextShapeData>;
    using PlacedDataPtr = std::unique_ptr<PlacedTextData>;

    /* implicit */ TextShape(DataPtr&& data);
    /* implicit */ TextShape(PlacedDataPtr&& data);
    // TODO: Remove
    /* implicit */ TextShape(DataPtr&& data, PlacedDataPtr&& placedData);

    /// The data.
    DataPtr data;
    PlacedDataPtr placedData;

    bool active = true;
    /// Gets labeled as "dirty" on font face change. Shaping needs to be called again.
    bool dirty = false;

    void deactivate();

    const priv::TextShapeData& getData() const;
    const PlacedTextData& getPlacedData() const;

    void onFontFaceChanged(const std::string& postScriptName);

    bool isPlaced() const;
};

}
