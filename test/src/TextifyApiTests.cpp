
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

        ASSERT_EQ(octopusData.type, octopus::Octopus::Type::ARTBOARD);
        ASSERT_TRUE(octopusData.content.has_value());

        ASSERT_TRUE(octopusData.content->layers.has_value());
        ASSERT_FALSE(octopusData.content->layers->empty());
    }

    static void comparePlacement(const textify::QuadCorners &pg1pos, const textify::QuadCorners &pg2pos) {
        ASSERT_EQ(pg1pos.topLeft.x, pg2pos.topLeft.x);
        ASSERT_EQ(pg1pos.topLeft.y, pg2pos.topLeft.y);
        ASSERT_EQ(pg1pos.topRight.x, pg2pos.topRight.x);
        ASSERT_EQ(pg1pos.topRight.y, pg2pos.topRight.y);
        ASSERT_EQ(pg1pos.bottomLeft.x, pg2pos.bottomLeft.x);
        ASSERT_EQ(pg1pos.bottomLeft.y, pg2pos.bottomLeft.y);
        ASSERT_EQ(pg1pos.bottomRight.x, pg2pos.bottomRight.x);
        ASSERT_EQ(pg1pos.bottomRight.y, pg2pos.bottomRight.y);
    }

    textify::ContextHandle context;

    const std::string fontsDirectory = std::string(FONTS_DIR);
    const std::string singleLetterOctopusPath = std::string(TESTING_OCTOPUS_DIR) + "SingleLetter.json";
    const std::string decorationsOctopusPath = std::string(TESTING_OCTOPUS_DIR) + "Decorations.json";
    const textify::FontSpecifier fontHelveticaNeue { "HelveticaNeue" };
};


TEST_F(TextifyApiTests, singleLetter) {
    using namespace textify;

    octopus::Octopus octopusData;
    readOctopusFile(singleLetterOctopusPath, octopusData);

    const octopus::Layer &textLayer = octopusData.content->layers->front();
    const nonstd::optional<octopus::Text> &text = textLayer.text;

    ASSERT_TRUE(text.has_value());

    addMissingFonts(*text);

    const TextShapeHandle textShape = shapePlacedText(context, *text);
    ASSERT_TRUE(textShape != nullptr);

    ASSERT_TRUE(textShape->placedData != nullptr);
    ASSERT_EQ(textShape->placedData->glyphs.size(), 1);
    ASSERT_EQ(textShape->placedData->decorations.size(), 0);
    ASSERT_TRUE(textShape->placedData->firstBaseline.has_value());
    ASSERT_EQ(*textShape->placedData->firstBaseline, 571.203125);
    ASSERT_EQ(textShape->placedData->glyphs.count(fontHelveticaNeue), 1);

    const PlacedGlyphs &pgs = textShape->placedData->glyphs.at(fontHelveticaNeue);
    ASSERT_EQ(pgs.size(), 1);

    const PlacedGlyphPtr &pg = pgs.front();
    ASSERT_EQ(pg->fontSize, 600);
    ASSERT_EQ(pg->glyphCodepoint, 60);
    ASSERT_EQ(pg->index, 0);
    comparePlacement(pg->placement, QuadCorners{
        Vector2f{1.0f, 142.0f},
        Vector2f{344.0f, 142.0f},
        Vector2f{1.0f, 571.0f},
        Vector2f{344.0f, 571.0f} });

    ode::BitmapPtr bitmap = nullptr;

    const DrawOptions drawOptions { 1.0f, std::nullopt };
    const Dimensions dimensions = getDrawBufferDimensions(context, textShape, drawOptions);

    bitmap = std::make_shared<ode::Bitmap>(ode::PixelFormat::RGBA, ode::Vector2i(dimensions.width, dimensions.height));
    bitmap->clear();

    const DrawTextResult drawResult = drawPlacedText(context, textShape, bitmap->pixels(), bitmap->width(), bitmap->height(), drawOptions);
    ASSERT_FALSE(drawResult.error);
}

TEST_F(TextifyApiTests, decorations) {
    using namespace textify;

    octopus::Octopus octopusData;
    readOctopusFile(decorationsOctopusPath, octopusData);

    const octopus::Layer &textLayer = octopusData.content->layers->front();
    const nonstd::optional<octopus::Text> &text = textLayer.text;

    ASSERT_TRUE(text.has_value());

    addMissingFonts(*text);

    const TextShapeHandle textShape = shapePlacedText(context, *text);
    ASSERT_TRUE(textShape != nullptr);

    ASSERT_TRUE(textShape->placedData != nullptr);
    ASSERT_EQ(textShape->placedData->glyphs.size(), 1);
    ASSERT_EQ(textShape->placedData->decorations.size(), 7);
    ASSERT_TRUE(textShape->placedData->firstBaseline.has_value());
    ASSERT_EQ(*textShape->placedData->firstBaseline, 47.59375);
    ASSERT_EQ(textShape->placedData->glyphs.count(fontHelveticaNeue), 1);

    const PlacedGlyphs &pgs = textShape->placedData->glyphs.at(fontHelveticaNeue);
    ASSERT_EQ(pgs.size(), 40);

    const PlacedDecorations &decorations = textShape->placedData->decorations;
    ASSERT_EQ(decorations[0]->type, PlacedDecoration::Type::UNDERLINE);
    ASSERT_EQ(decorations[1]->type, PlacedDecoration::Type::STRIKE_THROUGH);
    ASSERT_EQ(decorations[2]->type, PlacedDecoration::Type::UNDERLINE);
    ASSERT_EQ(decorations[3]->type, PlacedDecoration::Type::UNDERLINE);
    ASSERT_EQ(decorations[4]->type, PlacedDecoration::Type::STRIKE_THROUGH);
    ASSERT_EQ(decorations[5]->type, PlacedDecoration::Type::DOUBLE_UNDERLINE);
    ASSERT_EQ(decorations[6]->type, PlacedDecoration::Type::UNDERLINE);

    comparePlacement(decorations[4]->placement, QuadCorners{
        Vector2f{118.0f, 94.0f},
        Vector2f{208.0f, 94.0f},
        Vector2f{118.0f, 96.5f},
        Vector2f{208.0f, 96.5f} });
    comparePlacement(decorations[5]->placement, QuadCorners{
        Vector2f{118.0f, 116.0f},
        Vector2f{152.0f, 116.0f},
        Vector2f{118.0f, 122.25f},
        Vector2f{152.0f, 122.25f} });
    comparePlacement(decorations[6]->placement, QuadCorners{
        Vector2f{208.0f, 116.0f},
        Vector2f{428.0f, 116.0f},
        Vector2f{208.0f, 118.5f},
        Vector2f{428.0f, 118.5f} });

    ode::BitmapPtr bitmap = nullptr;

    const DrawOptions drawOptions { 1.0f, std::nullopt };
    const Dimensions dimensions = getDrawBufferDimensions(context, textShape, drawOptions);

    bitmap = std::make_shared<ode::Bitmap>(ode::PixelFormat::RGBA, ode::Vector2i(dimensions.width, dimensions.height));
    bitmap->clear();

    const DrawTextResult drawResult = drawPlacedText(context, textShape, bitmap->pixels(), bitmap->width(), bitmap->height(), drawOptions);
    ASSERT_FALSE(drawResult.error);
}
