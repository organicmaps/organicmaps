#pragma once

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL  // TODO: Remove this line after upgrading glm to the latest version.
#include <glm/gtx/rotate_vector.hpp>

namespace glsl
{
using glm::dot;
using glm::cross;
using glm::normalize;
using glm::length;
using glm::distance;

using glm::translate;
using glm::rotate;
using glm::scale;
using glm::transpose;
}  // namespace glsl
