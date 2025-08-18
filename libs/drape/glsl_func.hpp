#pragma once

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL  // TODO: Remove this line after upgrading glm to the latest version.
#include "std/glm_gtx_rotate_vector.hpp"

namespace glsl
{
using glm::cross;
using glm::distance;
using glm::dot;
using glm::length;
using glm::normalize;

using glm::rotate;
using glm::scale;
using glm::translate;
using glm::transpose;
}  // namespace glsl
