#pragma once

#include "drape/glsl_types.hpp"
#include "drape/binding_info.hpp"

#include "base/buffer_vector.hpp"

namespace gpu
{

struct BaseVertex
{
  using TPosition = glsl::vec3;
  using TPosition3d = glsl::vec4;
  using TNormal = glsl::vec2;
  using TNormal3d = glsl::vec3;
  using TTexCoord = glsl::vec2;
};

struct AreaVertex : BaseVertex
{
  AreaVertex();
  AreaVertex(TPosition const & position, TTexCoord const & colorTexCoord);

  TPosition m_position;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct Area3dVertex : BaseVertex
{
  Area3dVertex();
  Area3dVertex(TPosition const & position, const TPosition & normal, TTexCoord const & colorTexCoord);

  TPosition m_position;
  TNormal3d m_normal;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct SolidTexturingVertex : BaseVertex
{
  SolidTexturingVertex();
  SolidTexturingVertex(TPosition3d const & position, TNormal const & normal, TTexCoord const & colorTexCoord);

  TPosition3d m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

typedef buffer_vector<SolidTexturingVertex, 128> TSolidTexVertexBuffer;

struct TextStaticVertex : BaseVertex
{
  TextStaticVertex();
  TextStaticVertex(TTexCoord const & colorTexCoord, TTexCoord const & maskTexCoord);

  TTexCoord m_colorTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

typedef buffer_vector<TextStaticVertex, 128> TTextStaticVertexBuffer;

struct TextOutlinedStaticVertex : BaseVertex
{
public:
  TextOutlinedStaticVertex();
  TextOutlinedStaticVertex(TTexCoord const & colorTexCoord, TTexCoord const & outlineTexCoord,
                           TTexCoord const & maskTexCoord);

  TTexCoord m_colorTexCoord;
  TTexCoord m_outlineTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

typedef buffer_vector<TextOutlinedStaticVertex, 128> TTextOutlinedStaticVertexBuffer;

struct TextDynamicVertex : BaseVertex
{
  TextDynamicVertex();
  TextDynamicVertex(TPosition3d const & position, TNormal const & normal);

  TPosition3d m_position;
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
  using TNormal = glsl::vec3;
  using TMaskTexCoord = glsl::vec4;

  DashedLineVertex();
  DashedLineVertex(TPosition const & position, TNormal const & normal,
                   TTexCoord const & color, TMaskTexCoord const & mask);

  TPosition m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;
  TMaskTexCoord m_maskTexCoord;

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
