#pragma once

#include "drape/glsl_types.hpp"
#include "drape/binding_info.hpp"

#include "base/buffer_vector.hpp"

namespace gpu
{

struct BaseVertex
{
  typedef glsl::vec3 TPosition;
  typedef glsl::vec2 TNormal;
  typedef glsl::vec2 TTexCoord;
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
  LineVertex();
  LineVertex(TPosition const & position, TNormal const & normal,
             TTexCoord const & color, TTexCoord const & mask);

  TPosition m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct RouteVertex : BaseVertex
{
  RouteVertex();
  RouteVertex(TPosition const & position, TNormal const & normal);

  TPosition m_position;
  TNormal m_normal;

  static dp::BindingInfo const & GetBindingInfo();
};

} // namespace gpu
