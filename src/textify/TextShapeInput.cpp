
#include "TextShapeInput.h"

namespace textify {
namespace priv {

TextShapeInput::TextShapeInput(FormattedTextPtr text,
                             const FrameSizeOpt &frameSize,
                             const compat::Matrix3f& textTransform)
    : formattedText(std::move(text)),
      frameSize(frameSize),
      textTransform(textTransform),
      usedFaces(formattedText->collectUsedFaceNames())
{ }

} // namespace priv
} // namespace textify
