#pragma once

#include "drape/glsl_types.hpp"
#include "drape/binding_info.hpp"

#include "base/buffer_vector.hpp"

namespace gpu
{

struct BaseVertex
{
  using TPosition = glsl::vec3;
  using TNormal = glsl::vec2;
  using TTexCoord = glsl::vec2;
};

struct SolidTexturingVertex : BaseVertex
{
  SolidTexturingVertex();
  SolidTexturingVertex(TPosition const & position, TNormal const & normal, TTexCoord const & colorTexCoord);

  TPosition m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

typedef buffer_vector<SolidTexturingVertex, 128> TSolidTexVertexBuffer;

struct TextStaticVertex : BaseVertex
{
public:
  TextStaticVertex();
  TextStaticVertex(TPosition const & position, TTexCoord const & colorTexCoord,
                   TTexCoord const & outlineTexCoord, TTexCoord const & maskTexCoord);

  TPosition m_position;
  TTexCoord m_colorTexCoord;
  TTexCoord m_outlineTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

typedef buffer_vector<TextStaticVertex, 128> TTextStaticVertexBuffer;

struct TextDynamicVertex : BaseVertex
{
  TextDynamicVertex();
  TextDynamicVertex(TNormal const & normal);

  TNormal m_normal;

  static dp::BindingInfo const & GetBindingInfo();
  static uint32_t GetDynamicStreamID();
};

typedef buffer_vector<TextDynamicVertex, 128> TTextDynamicVertexBuffer;

struct LineVertex : BaseVertex
{
  using TNormal = glsl::vec3;

  LineVertex();
  LineVertex(TPosition const & position, TNormal const & normal, TTexCoord const & color);

  TPosition m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct DashedLineVertex : BaseVertex
{
  DashedLineVertex();
  DashedLineVertex(TPosition const & position, TNormal const & normal,
                   TTexCoord const & color, TTexCoord const & mask);

  TPosition m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct RouteVertex : BaseVertex
{
  typedef glsl::vec3 TLength;

  RouteVertex();
  RouteVertex(TPosition const & position, TNormal const & normal, TLength const & length);

  TPosition m_position;
  TNormal m_normal;
  TLength m_length;

  static dp::BindingInfo const & GetBindingInfo();
};

} // namespace gpu
