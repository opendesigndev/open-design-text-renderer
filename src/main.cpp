#include "compat/basic-types.h"
#include "compat/Bitmap.hpp"
#include "compat/png.h"

#include "cli/Canvas.h"
#include "cli/SimpleRenderer.h"

#include "octopus/fill.h"
#include "octopus/general.h"
#include "textify/Context.h"
#include "textify/base-types.h"
#include "textify/textify-api.h"

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

void printMatrix(const std::string& label, const textify::Matrix3f& r) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            fmt::print("{} ", r.m[j][i]);
        }
        fmt::print("\n");
    }
}


class TextifyContext
{
public:
    explicit TextifyContext(const textify::ContextOptions& opts)
        : handle(textify::createContext(opts))
    { }

    ~TextifyContext() {
        textify::destroyContext(handle);
    }

    /* implicit */ operator textify::ContextHandle() const {
        return handle;
    }

private:
    textify::ContextHandle handle;
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
    textify::ContextOptions ctxOptions;
    ctxOptions.errorFunc = [](const std::string &msg) {
        fmt::print(fg(fmt::color::white) | bg(fmt::color::crimson) | fmt::emphasis::bold, "[ERROR] {}", msg);
        fmt::print("\n");
    };
    ctxOptions.warnFunc = [](const std::string &msg) { fmt::print("[WARN] {}\n", msg); };
    ctxOptions.infoFunc = [](const std::string &msg) { fmt::print("[INFO] {}\n", msg); };

    auto ctx = TextifyContext(ctxOptions);
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

    textify::addFontFile(ctx, "IBMPlexSans", "", "fonts/IBMPlexSans-Regular.ttf", false);
    textify::addFontFile(ctx, "IBMPlexSans-Regular", "", "fonts/IBMPlexSans-Regular.ttf", false);
    for (const auto& missing : textify::listMissingFonts(ctx, text)) {
        fmt::print("missing font: {}\n", missing.c_str());
    }

    std::vector<textify::TextShape*> shapes;
    for (int i = 0; i < 64; ++i) {
        shapes.emplace_back(textify::shapeText(ctx, text));
    }

    std::vector<textify::TextShape*> toRemove;
    for (int i = 0; i < shapes.size(); ++i) {
        if (i % 2) {
            toRemove.emplace_back(shapes[i]);
        }
    }

    textify::destroyTextShapes(ctx, toRemove.data(), toRemove.size());

    // textify::addFontFile(ctx, "IBMPlexSans-Regular", "Avenir-Black", "fonts/Avenir.ttc", false);

    auto textShape = shapes[0];

    auto scale = 2.0f;

    auto bounds = textify::getBounds(ctx, textShape);
    textify::utils::printRect("text bounds:", bounds);

    // width *= scale;
    // height *= scale;

    auto viewArea = textify::Rectangle{141, 151, 80, 117};
    textify::DrawOptions drawOptions;
    drawOptions.scale = scale;
    //drawOptions.viewArea = viewArea;

    auto [width, height] = textify::getDrawBufferDimensions(ctx, textShape, drawOptions);

    auto len = 4 * width * height;
    uint32_t* pixels = (uint32_t*)malloc(len);
    memset(pixels, 0, len);

    fmt::print("draw buffer dimensions: {} x {}\n", width, height);

    auto drawResult = textify::drawText(ctx, textShape, pixels, width, height, drawOptions);

    textify::utils::printRect("drawResult.bounds:", drawResult.bounds);
    printMatrix("drawResult.bounds:", drawResult.transform);

    auto bmp = textify::compat::BitmapRGBA{textify::compat::BitmapRGBA::Wrap::WRAP_NO_OWN, pixels, width, height};

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

    auto renderer = textify::cli::SimpleRenderer{octopus, 1.0f};
    renderer.renderToFile("preview.png");

    return true;
}

int main(int argc, char* argv[]) {
    // printf("usage: textify-cli <octopus-file>\n");

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
