#include "compat/basic-types.h"
#include "compat/Bitmap.hpp"
#include "compat/png.h"

#include "cli/Canvas.h"
#include "cli/SimpleRenderer.h"

#include <open-design-text-renderer/text-renderer-api.h>
#include "octopus/fill.h"
#include "octopus/general.h"
#include "text-renderer/Context.h"
#include "text-renderer/base-types.h"

#include "utils/fmt.h"
#include "vendor/fmt/color.h"
#include "vendor/fmt/core.h"

#include <octopus/octopus.h>
#include <octopus/parser.h>
#include <octopus/text.h>

#include <cstdio>
#include <memory>
#include <type_traits>
#include <vector>

void printMatrix(const std::string& label, const odtr::Matrix3f& r) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            fmt::print("{} ", r.m[j][i]);
        }
        fmt::print("\n");
    }
}


class TextRendererContext
{
public:
    explicit TextRendererContext(const odtr::ContextOptions& opts)
        : handle(odtr::createContext(opts))
    { }

    ~TextRendererContext() {
        odtr::destroyContext(handle);
    }

    /* implicit */ operator odtr::ContextHandle() const {
        return handle;
    }

private:
    odtr::ContextHandle handle;
};

struct File
{
    explicit File(const std::string& filename)
        : fp(fopen(filename.c_str(), "r"))
    { }

    ~File()
    {
        fclose(fp);
    }

    std::vector<char> read() const {
        std::vector<char> data;

        fseek(fp, 0, SEEK_END);
        auto length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        data.resize(length + 1);

        auto read = fread(data.data(), 1, length, fp);
        data[length] = '\0';

        return data;
    }

    FILE* fp = nullptr;
};

bool minimalExample() {
    odtr::ContextOptions ctxOptions;
    ctxOptions.errorFunc = [](const std::string &msg) {
        fmt::print(fg(fmt::color::white) | bg(fmt::color::crimson) | fmt::emphasis::bold, "[ERROR] {}", msg);
        fmt::print("\n");
    };
    ctxOptions.warnFunc = [](const std::string &msg) { fmt::print("[WARN] {}\n", msg); };
    ctxOptions.infoFunc = [](const std::string &msg) { fmt::print("[INFO] {}\n", msg); };

    auto ctx = TextRendererContext(ctxOptions);
    if (!ctx) {
        return false;
    }

    octopus::Fill fill;
    fill.type = octopus::Fill::Type::COLOR;
    fill.color = octopus::Color { 0.9f, 0.1f, 0.1, 1.0f };

    octopus::TextStyle style1;
    style1.fontSize = 36;
    style1.font = octopus::Font();
    style1.font->postScriptName = "IBMPlexSans-Regular";
    style1.fills = {fill};

    octopus::TextStyle style2;
    style2.fontSize = 36;
    style2.font = octopus::Font();
    style2.font->postScriptName = "IBMPlexSans-Regular";

    octopus::Text text1;
    text1.value = "First line\nSecond line";
    text1.defaultStyle = style1;
    text1.styles = std::vector<octopus::StyleRange> { octopus::StyleRange{style2, {{0, 10}}} };
    text1.transform[0] = 0.7771459817886353;
    text1.transform[1] = -0.6293203830718994;
    text1.transform[2] = 0.6293203830718994;
    text1.transform[3] = 0.7771459817886353;
    text1.transform[4] = 187.2850799560547;
    text1.transform[5] = 314.066650390625;
    text1.frame = octopus::TextFrame();
    text1.frame->mode = octopus::TextFrame::Mode::AUTO_WIDTH;
    text1.frame->size = octopus::Dimensions { 125, 31 };
    text1.baselinePolicy = octopus::Text::BaselinePolicy::SET;

    const auto& text = text1;

    odtr::addFontFile(ctx, "IBMPlexSans", "", "fonts/IBMPlexSans-Regular.ttf", false);
    odtr::addFontFile(ctx, "IBMPlexSans-Regular", "", "fonts/IBMPlexSans-Regular.ttf", false);
    for (const auto& missing : odtr::listMissingFonts(ctx, text)) {
        fmt::print("missing font: {}\n", missing.c_str());
    }

    std::vector<odtr::TextShape*> shapes;
    for (int i = 0; i < 64; ++i) {
        shapes.emplace_back(odtr::shapeText(ctx, text));
    }

    std::vector<odtr::TextShape*> toRemove;
    for (int i = 0; i < shapes.size(); ++i) {
        if (i % 2) {
            toRemove.emplace_back(shapes[i]);
        }
    }

    odtr::destroyTextShapes(ctx, toRemove.data(), toRemove.size());

    // odtr::addFontFile(ctx, "IBMPlexSans-Regular", "Avenir-Black", "fonts/Avenir.ttc", false);

    auto textShape = shapes[0];

    auto scale = 2.0f;

    auto bounds = odtr::getBounds(ctx, textShape);
    odtr::utils::printRect("text bounds:", bounds);

    // width *= scale;
    // height *= scale;

    auto viewArea = odtr::Rectangle{141, 151, 80, 117};
    odtr::DrawOptions drawOptions;
    drawOptions.scale = scale;
    //drawOptions.viewArea = viewArea;

    auto [width, height] = odtr::getDrawBufferDimensions(ctx, textShape, drawOptions);

    auto len = 4 * width * height;
    uint32_t* pixels = (uint32_t*)malloc(len);
    memset(pixels, 0, len);

    fmt::print("draw buffer dimensions: {} x {}\n", width, height);

    auto drawResult = odtr::drawText(ctx, textShape, pixels, width, height, drawOptions);

    odtr::utils::printRect("drawResult.bounds:", drawResult.bounds);
    printMatrix("drawResult.bounds:", drawResult.transform);

    auto bmp = odtr::compat::BitmapRGBA{odtr::compat::BitmapRGBA::Wrap::WRAP_NO_OWN, pixels, width, height};

    savePngImage(bmp, "preview.png");

    return true;
}

bool runOctopusRenderer(const std::string& filename) {

    auto f = File{filename.c_str()};
    auto raw = f.read();

    octopus::Octopus octopus;
    auto err = octopus::Parser::parse(octopus, raw.data());
    if (err != octopus::Parser::OK) {
        fmt::print("error: {}\n", (int)err.type);
        return false;
    } else {
        fmt::print("# layers: {}\n", octopus.content.get()->layers.value().size());
    }

    auto renderer = odtr::cli::SimpleRenderer{octopus, 1.0f};
    renderer.renderToFile("preview.png");

    return true;
}

int main(int argc, char* argv[]) {
    // printf("usage: text-renderer-cli <octopus-file>\n");

    std::vector<std::string> args(argv, argv + argc);
    if (args.size() >= 2) {
        if (runOctopusRenderer(args[1])) {
            return 0;
        }
        return 1;
    }

    minimalExample();

    return 0;
}
