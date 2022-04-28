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

    std::unique_ptr<priv::TextShapeData> data;
    bool active = true;
    bool dirty = false;

    void deactivate();

    const priv::TextShapeData& getData() const;

    void onFontFaceChanged(const std::string& postScriptName);
};

}
