#pragma once

#include "../glsl_types.hpp"
#include "../binding_info.hpp"

#include "../../base/buffer_vector.hpp"

namespace gpu
{

struct SolidTexturingVertex
{
  SolidTexturingVertex();
  SolidTexturingVertex(glsl::vec3 const & position, glsl::vec2 const & normal, glsl::vec2 const & colorTexCoord);

  glsl::vec3 m_position;
  glsl::vec2 m_normal;
  glsl::vec2 m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

typedef buffer_vector<SolidTexturingVertex, 128> TSolidTexVertexBuffer;

struct TextStaticVertex
{
public:
  TextStaticVertex() = default;
  TextStaticVertex(glsl::vec3 position, glsl::vec2 colorTexCoord, glsl::vec2 outlineTexCoord, glsl::vec2 maskTexCoord)
    : m_position(position)
    , m_colorTexCoord(colorTexCoord)
    , m_outlineTexCoord(outlineTexCoord)
    , m_maskTexCoord(maskTexCoord)
  {
  }

  glsl::vec3 m_position;
  glsl::vec2 m_colorTexCoord;
  glsl::vec2 m_outlineTexCoord;
  glsl::vec2 m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

typedef buffer_vector<TextStaticVertex, 128> TTextStaticVertexBuffer;

struct TextDynamicVertex
{
  TextDynamicVertex() = default;
  TextDynamicVertex(glsl::vec2 normal) : m_normal(normal) {}

  glsl::vec2 m_normal;

  static dp::BindingInfo const & GetBindingInfo();
  static uint32_t GetDynamicStreamID();
};

typedef buffer_vector<TextDynamicVertex, 128> TTextDynamicVertexBuffer;

} // namespace gpu
