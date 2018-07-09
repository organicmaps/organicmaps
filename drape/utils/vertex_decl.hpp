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

struct HatchingAreaVertex : BaseVertex
{
  using TMaskTexCoord = glsl::vec2;

  HatchingAreaVertex();
  HatchingAreaVertex(TPosition const & position, TTexCoord const & colorTexCoord,
                     TMaskTexCoord const & maskTexCoord);

  TPosition m_position;
  TTexCoord m_colorTexCoord;
  TMaskTexCoord m_maskTexCoord;

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

using TSolidTexVertexBuffer = buffer_vector<SolidTexturingVertex, 128>;

struct MaskedTexturingVertex : BaseVertex
{
  MaskedTexturingVertex();
  MaskedTexturingVertex(TPosition3d const & position, TNormal const & normal,
                        TTexCoord const & colorTexCoord, TTexCoord const & maskTexCoord);
  TPosition3d m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct TextStaticVertex : BaseVertex
{
  TextStaticVertex();
  TextStaticVertex(TTexCoord const & colorTexCoord, TTexCoord const & maskTexCoord);

  TTexCoord m_colorTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

using TTextStaticVertexBuffer = buffer_vector<TextStaticVertex, 128>;

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

using TTextOutlinedStaticVertexBuffer = buffer_vector<TextOutlinedStaticVertex, 128>;

struct TextDynamicVertex : BaseVertex
{
  TextDynamicVertex();
  TextDynamicVertex(TPosition3d const & position, TNormal const & normal);

  TPosition3d m_position;
  TNormal m_normal;

  static dp::BindingInfo const & GetBindingInfo();
  static uint32_t GetDynamicStreamID();
};

using TTextDynamicVertexBuffer = buffer_vector<TextDynamicVertex, 128>;

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
  using TLength = glsl::vec3;
  using TColor = glsl::vec4;

  RouteVertex();
  RouteVertex(TPosition const & position, TNormal const & normal,
              TLength const & length, TColor const & color);

  TPosition m_position;
  TNormal m_normal;
  TLength m_length;
  TColor m_color;

  static dp::BindingInfo const & GetBindingInfo();
};

struct RouteMarkerVertex : BaseVertex
{
  using TPosition = glsl::vec4;
  using TNormal = glsl::vec3;
  using TColor = glsl::vec4;

  RouteMarkerVertex();
  RouteMarkerVertex(TPosition const & position, TNormal const & normal,
                    TColor const & color);

  TPosition m_position;
  TNormal m_normal;
  TColor m_color;

  static dp::BindingInfo const & GetBindingInfo();
};

struct ColoredSymbolVertex : BaseVertex
{
  using TNormal = glsl::vec4;
  using TTexCoord = glsl::vec4;

  ColoredSymbolVertex();
  ColoredSymbolVertex(TPosition const & position, TNormal const & normal,
                      TTexCoord const & colorTexCoord);

  TPosition m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

} // namespace gpu
