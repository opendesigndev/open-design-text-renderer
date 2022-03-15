// compat: core/src/model/text-format.h

#include "text-format.h"

const int FormatModifier::FACE = 0x1;
const int FormatModifier::SIZE = 0x2;
const int FormatModifier::LINE_HEIGHT = 0x4;
const int FormatModifier::LETTER_SPACING = 0x8;
const int FormatModifier::PARAGRAPH_SPACING = 0x10;
const int FormatModifier::COLOR = 0x20;
const int FormatModifier::DECORATION = 0x40;
const int FormatModifier::ALIGN = 0x80;
const int FormatModifier::KERNING = 0x100;
const int FormatModifier::LIGATURES = 0x200;
const int FormatModifier::UPPERCASE = 0x400;
const int FormatModifier::LOWERCASE = 0x800;
const int FormatModifier::PARAGRAPH_INDENT = 0x1000;
const int FormatModifier::TYPE_FEATURE = 0x2000;
const int FormatModifier::TAB_STOPS = 0x4000;

bool operator==(const TypeFeature& a, const TypeFeature& b)
{
    return a.tag == b.tag && a.value == b.value;
}

