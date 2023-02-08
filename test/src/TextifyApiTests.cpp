
#include <memory>
#include <gtest/gtest.h>

#include <octopus/octopus.h>
#include <octopus/parser.h>
#include <octopus/validator.h>
#include <ode-essentials.h>
#include <ode-renderer.h>

#include <textify/textify-api.h>


namespace {
static textify::ContextOptions textifyContextOptions() {
    textify::ContextOptions options;
    options.errorFunc = [](const std::string &message) {
        fprintf(stderr, "Textify error: %s\n", message.c_str());
    };
    options.warnFunc = [](const std::string &message) {
        fprintf(stderr, "Textify warning: %s\n", message.c_str());
    };
    options.infoFunc = [](const std::string &) { };
    return options;
}
}


//! TextifyApiTests class tests.
class TextifyApiTests : public ::testing::Test {
protected:
    virtual void SetUp() override {
        context = textify::createContext(textifyContextOptions());
    }

    textify::ContextHandle context;

    const std::string fontsDirectory = std::string(FONTS_DIR);
    const std::string singleLetterOctopusPath = std::string(TESTING_OCTOPUS_DIR) + "SingleLetter.json";
};


TEST_F(TextifyApiTests, always) {
    std::string octopusJson;
    ASSERT_TRUE(ode::readFile(singleLetterOctopusPath, octopusJson));

    octopus::Octopus octopusData;
    const octopus::Parser::Error parseError = octopus::Parser::parse(octopusData, octopusJson.c_str());
    ASSERT_FALSE(parseError);

    std::set<std::string> layerIds;
    std::string validationError;
    ASSERT_TRUE(octopus::validate(octopusData, layerIds, &validationError));

    ASSERT_EQ(octopusData.type, octopus::Octopus::Type::ARTBOARD);
    ASSERT_TRUE(octopusData.content.has_value());

    ASSERT_TRUE(octopusData.content->layers.has_value());
    ASSERT_FALSE(octopusData.content->layers->empty());

    const octopus::Layer &textLayer = octopusData.content->layers->front();
    const nonstd::optional<octopus::Text> &text = textLayer.text;

    ASSERT_TRUE(text.has_value());

    const std::vector<std::string> missingFonts = textify::listMissingFonts(context, *text);
    for (const std::string &missingFont : missingFonts) {
        const bool isAdded =
            textify::addFontFile(context, missingFont, std::string(), (std::string) (fontsDirectory+(missingFont+".ttf")), false) ||
            textify::addFontFile(context, missingFont, std::string(), (std::string) (fontsDirectory+(missingFont+".otf")), false);
        ASSERT_TRUE(isAdded);
    }

    const textify::TextShapeHandle textShape = textify::shapePlacedText(context, *text);
    ASSERT_TRUE(textShape != nullptr);

    ode::BitmapPtr bitmap = nullptr;

    const textify::DrawOptions drawOptions { 1.0f, std::nullopt };
    const textify::Dimensions dimensions = textify::getDrawBufferDimensions(context, textShape, drawOptions);

    bitmap = std::make_shared<ode::Bitmap>(ode::PixelFormat::RGBA, ode::Vector2i(dimensions.width, dimensions.height));
    bitmap->clear();

    const textify::DrawTextResult drawResult = textify::drawPlacedText(context, textShape, bitmap->pixels(), bitmap->width(), bitmap->height(), drawOptions);
    ASSERT_FALSE(drawResult.error);
}
