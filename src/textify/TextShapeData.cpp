
#include "TextShapeData.h"

namespace textify {
namespace priv {

TextShapeData::TextShapeData(FormattedTextPtr text,
                             FrameSizeOpt frameSize,
                             const compat::Matrix3f& textTransform,
                             ParagraphShapes&& shapes,
                             const compat::FRectangle& boundsNoTransform,
                             const compat::FRectangle& boundsTransformed,
                             float baseline)
    : formattedText(std::move(text)),
      frameSize(frameSize),
      textTransform(textTransform),
      usedFaces(formattedText->collectUsedFaceNames()),
      paragraphShapes(std::move(shapes)),
      textBoundsNoTransform(boundsNoTransform),
      textBoundsTransformed(boundsTransformed),
      baseline(baseline)
{ }

} // namespace priv
} // namespace textify
