
#include <memory>
#include <gtest/gtest.h>

#include <octopus/octopus.h>
#include <octopus/parser.h>
#include <octopus/validator.h>
#include <ode-essentials.h>
#include <ode-renderer.h>

#include <textify/textify-api.h>
#include <textify/PlacedTextData.h>

#include "textify/TextShape.h"


namespace {
static odtr::ContextOptions textifyContextOptions() {
    odtr::ContextOptions options;
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
        context = odtr::createContext(textifyContextOptions());
    }

    void addMissingFonts(const octopus::Text &text) {
        const std::vector<std::string> missingFonts = listMissingFonts(context, text);
        for (const std::string &missingFont : missingFonts) {
            const bool isAdded =
                addFontFile(context, missingFont, std::string(), (std::string) (fontsDirectory+(missingFont+".ttf")), false) ||
                addFontFile(context, missingFont, std::string(), (std::string) (fontsDirectory+(missingFont+".otf")), false);
            ASSERT_TRUE(isAdded);
        }
    }

    static void readOctopusFile(const std::string &octopusFilePath, octopus::Octopus &octopusData) {
        std::string octopusJson;
        ASSERT_TRUE(ode::readFile(octopusFilePath, octopusJson));

        const octopus::Parser::Error parseError = octopus::Parser::parse(octopusData, octopusJson.c_str());
        ASSERT_FALSE(parseError);

        std::set<std::string> layerIds;
        std::string validationError;
        ASSERT_TRUE(octopus::validate(octopusData, layerIds, &validationError));

        ASSERT_EQ(octopusData.type, octopus::Octopus::Type::OCTOPUS_COMPONENT);
        ASSERT_TRUE(octopusData.content.has_value());

        ASSERT_TRUE(octopusData.content->layers.has_value());
        ASSERT_FALSE(octopusData.content->layers->empty());
    }

    odtr::ContextHandle context;

    const std::string fontsDirectory = std::string(FONTS_DIR);
    const std::string singleLetterOctopusPath = std::string(TESTING_OCTOPUS_DIR) + "SingleLetter.json";
    const std::string decorationsOctopusPath = std::string(TESTING_OCTOPUS_DIR) + "Decorations.json";
    const odtr::FontSpecifier fontHelveticaNeue { "HelveticaNeue" };
};


TEST_F(TextifyApiTests, singleLetter) {
    using namespace odtr;

    octopus::Octopus octopusData;
    readOctopusFile(singleLetterOctopusPath, octopusData);

    const octopus::Layer &textLayer = octopusData.content->layers->front();
    const nonstd::optional<octopus::Text> &text = textLayer.text;

    ASSERT_TRUE(text.has_value());

    addMissingFonts(*text);

    const TextShapeHandle textShape = shapeText(context, *text);
    ASSERT_TRUE(textShape != nullptr);

    ASSERT_TRUE(textShape->data != nullptr);
    ASSERT_EQ(textShape->data->glyphs.size(), 1);
    ASSERT_EQ(textShape->data->decorations.size(), 0);
    ASSERT_EQ(textShape->data->baseline, 571.203125);
    ASSERT_EQ(textShape->data->glyphs.count(fontHelveticaNeue), 1);

    const PlacedGlyphs &pgs = textShape->data->glyphs.at(fontHelveticaNeue);
    ASSERT_EQ(pgs.size(), 1);

    const PlacedGlyphPtr &pg = pgs.front();
    ASSERT_EQ(pg->fontSize, 600);
    ASSERT_EQ(pg->codepoint, 60);
    ASSERT_EQ(pg->index, 0);
    ASSERT_EQ(pg->originPosition.x, 0.0f);
    ASSERT_EQ(pg->originPosition.y, 571.203125f);

    ode::BitmapPtr bitmap = nullptr;

    const DrawOptions drawOptions { 1.0f, std::nullopt };
    const Dimensions dimensions = getDrawBufferDimensions(context, textShape, drawOptions);

    bitmap = std::make_shared<ode::Bitmap>(ode::PixelFormat::RGBA, ode::Vector2i(dimensions.width, dimensions.height));
    bitmap->clear();

    const DrawTextResult drawResult = drawText(context, textShape, bitmap->pixels(), bitmap->width(), bitmap->height(), drawOptions);
    ASSERT_FALSE(drawResult.error);
}

TEST_F(TextifyApiTests, decorations) {
    using namespace odtr;

    octopus::Octopus octopusData;
    readOctopusFile(decorationsOctopusPath, octopusData);

    const octopus::Layer &textLayer = octopusData.content->layers->front();
    const nonstd::optional<octopus::Text> &text = textLayer.text;

    ASSERT_TRUE(text.has_value());

    addMissingFonts(*text);

    const TextShapeHandle textShape = shapeText(context, *text);
    ASSERT_TRUE(textShape != nullptr);

    ASSERT_TRUE(textShape->data != nullptr);
    ASSERT_EQ(textShape->data->glyphs.size(), 1);
    ASSERT_EQ(textShape->data->decorations.size(), 7);
    ASSERT_EQ(textShape->data->baseline, 47.59375);
    ASSERT_EQ(textShape->data->glyphs.count(fontHelveticaNeue), 1);

    const PlacedGlyphs &pgs = textShape->data->glyphs.at(fontHelveticaNeue);
    ASSERT_EQ(pgs.size(), 40);

    const PlacedDecorations &decorations = textShape->data->decorations;
    ASSERT_EQ(decorations[0]->type, PlacedDecoration::Type::UNDERLINE);
    ASSERT_EQ(decorations[1]->type, PlacedDecoration::Type::STRIKE_THROUGH);
    ASSERT_EQ(decorations[2]->type, PlacedDecoration::Type::UNDERLINE);
    ASSERT_EQ(decorations[3]->type, PlacedDecoration::Type::UNDERLINE);
    ASSERT_EQ(decorations[4]->type, PlacedDecoration::Type::STRIKE_THROUGH);
    ASSERT_EQ(decorations[5]->type, PlacedDecoration::Type::DOUBLE_UNDERLINE);
    ASSERT_EQ(decorations[6]->type, PlacedDecoration::Type::UNDERLINE);

    ode::BitmapPtr bitmap = nullptr;

    const DrawOptions drawOptions { 1.0f, std::nullopt };
    const Dimensions dimensions = getDrawBufferDimensions(context, textShape, drawOptions);

    bitmap = std::make_shared<ode::Bitmap>(ode::PixelFormat::RGBA, ode::Vector2i(dimensions.width, dimensions.height));
    bitmap->clear();

    const DrawTextResult drawResult = drawText(context, textShape, bitmap->pixels(), bitmap->width(), bitmap->height(), drawOptions);
    ASSERT_FALSE(drawResult.error);
}
