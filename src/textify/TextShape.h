#pragma once

#include <memory>
#include <string>

namespace textify {

namespace priv {
struct TextShapeData;
}

struct TextShape
{
    using DataPtr = std::unique_ptr<priv::TextShapeData>;

    /* implicit */ TextShape(DataPtr&& data);

    /// The data.
    std::unique_ptr<priv::TextShapeData> data;

    bool active = true;
    /// Gets labeled as "dirty" on font face change. Shaping needs to be called again.
    bool dirty = false;

    void deactivate();

    const priv::TextShapeData& getData() const;

    void onFontFaceChanged(const std::string& postScriptName);
};

}
