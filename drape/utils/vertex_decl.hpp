#pragma once

#include "drape/binding_info.hpp"
#include "drape/glsl_types.hpp"

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

template <class T>
using VBUnknownSizeT = buffer_vector<T, 128>;
template <class T>
using VBReservedSizeT = std::vector<T>;

struct AreaVertex : BaseVertex
{
  AreaVertex() = default;
  AreaVertex(TPosition const & position, TTexCoord const & colorTexCoord);

  TPosition m_position;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct Area3dVertex : BaseVertex
{
  Area3dVertex() = default;
  Area3dVertex(TPosition const & position, TPosition const & normal, TTexCoord const & colorTexCoord);

  TPosition m_position;
  TNormal3d m_normal;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct HatchingAreaVertex : BaseVertex
{
  using TMaskTexCoord = glsl::vec2;

  HatchingAreaVertex() = default;
  HatchingAreaVertex(TPosition const & position, TTexCoord const & colorTexCoord, TMaskTexCoord const & maskTexCoord);

  TPosition m_position;
  TTexCoord m_colorTexCoord;
  TMaskTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct SolidTexturingVertex : BaseVertex
{
  SolidTexturingVertex() = default;
  SolidTexturingVertex(TPosition3d const & position, TNormal const & normal, TTexCoord const & colorTexCoord);

  TPosition3d m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct MaskedTexturingVertex : BaseVertex
{
  MaskedTexturingVertex() = default;
  MaskedTexturingVertex(TPosition3d const & position, TNormal const & normal, TTexCoord const & colorTexCoord,
                        TTexCoord const & maskTexCoord);
  TPosition3d m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

struct TextStaticVertex : BaseVertex
{
  TextStaticVertex() = default;
  TextStaticVertex(TTexCoord const & colorTexCoord, TTexCoord const & maskTexCoord);

  TTexCoord m_colorTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

using TTextStaticVertexBuffer = VBReservedSizeT<TextStaticVertex>;

struct TextOutlinedStaticVertex : BaseVertex
{
public:
  TextOutlinedStaticVertex() = default;
  TextOutlinedStaticVertex(TTexCoord const & colorTexCoord, TTexCoord const & outlineTexCoord,
                           TTexCoord const & maskTexCoord);

  TTexCoord m_colorTexCoord;
  TTexCoord m_outlineTexCoord;
  TTexCoord m_maskTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

using TTextOutlinedStaticVertexBuffer = VBReservedSizeT<TextOutlinedStaticVertex>;

struct TextDynamicVertex : BaseVertex
{
  TextDynamicVertex() = default;
  TextDynamicVertex(TPosition3d const & position, TNormal const & normal);

  TPosition3d m_position;
  TNormal m_normal;

  static dp::BindingInfo const & GetBindingInfo();
  static uint32_t GetDynamicStreamID();
};

using TTextDynamicVertexBuffer = VBReservedSizeT<TextDynamicVertex>;

struct LineVertex : BaseVertex
{
  using TNormal = glsl::vec3;

  LineVertex() = default;
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

  DashedLineVertex() = default;
  DashedLineVertex(TPosition const & position, TNormal const & normal, TTexCoord const & color,
                   TMaskTexCoord const & mask);

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

  RouteVertex() = default;
  RouteVertex(TPosition const & position, TNormal const & normal, TLength const & length, TColor const & color);

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

  RouteMarkerVertex() = default;
  RouteMarkerVertex(TPosition const & position, TNormal const & normal, TColor const & color);

  TPosition m_position;
  TNormal m_normal;
  TColor m_color;

  static dp::BindingInfo const & GetBindingInfo();
};

struct ColoredSymbolVertex : BaseVertex
{
  using TNormal = glsl::vec4;
  using TTexCoord = glsl::vec4;

  ColoredSymbolVertex() = default;
  ColoredSymbolVertex(TPosition const & position, TNormal const & normal, TTexCoord const & colorTexCoord);

  TPosition m_position;
  TNormal m_normal;
  TTexCoord m_colorTexCoord;

  static dp::BindingInfo const & GetBindingInfo();
};

}  // namespace gpu
