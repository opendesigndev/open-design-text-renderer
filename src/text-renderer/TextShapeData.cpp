
#include "TextShapeData.h"

namespace odtr {
namespace priv {

TextShapeData::TextShapeData(ParagraphShapes&& shapes,
                             const compat::FRectangle& boundsNoTransform,
                             const compat::FRectangle& boundsTransformed,
                             float baseline)
    : paragraphShapes(std::move(shapes)),
      textBoundsNoTransform(boundsNoTransform),
      textBoundsTransformed(boundsTransformed),
      baseline(baseline)
{ }

} // namespace priv
} // namespace odtr
