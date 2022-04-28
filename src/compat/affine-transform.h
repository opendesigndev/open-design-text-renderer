
#pragma once

#include "basic-types.h"

namespace textify {
namespace compat {

Matrix3f rotationMatrix(float angle, Vector2f center = { });
Matrix3f scaleMatrix(float xScale, float yScale, Vector2f center = { });
Matrix3f translationMatrix(Vector2f translation);

FRectangle transform(const FRectangle& r, const Matrix3f& m);

/**
 * Returns true if a 2D linear sub-matrix of a given matrix is an identify.
 */
bool isLinearIdentity(const Matrix3f& m);

bool hasTranslation(const Matrix3f& m);

float deconstructRotation(const Matrix3f& transformation, bool &flip);

} // namespace compat
} // namespace textify
