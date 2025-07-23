#include "drape/utils/vertex_decl.hpp"

namespace gpu
{
namespace
{
enum VertexType
{
  Area,
  Area3d,
  HatchingArea,
  SolidTexturing,
  MaskedTexturing,
  TextStatic,
  TextOutlinedStatic,
  TextDynamic,
  Line,
  DashedLine,
  Route,
  RouteMarker,
  ColoredSymbol,
  TypeCount
};

struct BindingNode
{
  dp::BindingInfo m_info;
  bool m_inited = false;
};

typedef dp::BindingInfo (*TInitFunction)();

dp::BindingInfo AreaBindingInit()
{
  static_assert(sizeof(AreaVertex) == (sizeof(AreaVertex::TPosition) + sizeof(AreaVertex::TTexCoord)), "");

  dp::BindingFiller<AreaVertex> filler(2);
  filler.FillDecl<AreaVertex::TPosition>("a_position");
  filler.FillDecl<AreaVertex::TTexCoord>("a_colorTexCoords");

  return filler.m_info;
}

dp::BindingInfo Area3dBindingInit()
{
  static_assert(sizeof(Area3dVertex) == (sizeof(Area3dVertex::TPosition) + sizeof(Area3dVertex::TNormal3d) +
                                         sizeof(Area3dVertex::TTexCoord)),
                "");

  dp::BindingFiller<Area3dVertex> filler(3);
  filler.FillDecl<Area3dVertex::TPosition>("a_position");
  filler.FillDecl<Area3dVertex::TNormal3d>("a_normal");
  filler.FillDecl<Area3dVertex::TTexCoord>("a_colorTexCoords");

  return filler.m_info;
}

dp::BindingInfo HatchingAreaBindingInit()
{
  static_assert(
      sizeof(HatchingAreaVertex) == (sizeof(HatchingAreaVertex::TPosition) + sizeof(HatchingAreaVertex::TTexCoord) +
                                     sizeof(HatchingAreaVertex::TMaskTexCoord)),
      "");

  dp::BindingFiller<HatchingAreaVertex> filler(3);
  filler.FillDecl<HatchingAreaVertex::TPosition>("a_position");
  filler.FillDecl<HatchingAreaVertex::TNormal>("a_colorTexCoords");
  filler.FillDecl<HatchingAreaVertex::TMaskTexCoord>("a_maskTexCoords");
  return filler.m_info;
}

dp::BindingInfo SolidTexturingBindingInit()
{
  static_assert(
      sizeof(SolidTexturingVertex) == (sizeof(SolidTexturingVertex::TPosition3d) +
                                       sizeof(SolidTexturingVertex::TNormal) + sizeof(SolidTexturingVertex::TTexCoord)),
      "");

  dp::BindingFiller<SolidTexturingVertex> filler(3);
  filler.FillDecl<SolidTexturingVertex::TPosition3d>("a_position");
  filler.FillDecl<SolidTexturingVertex::TNormal>("a_normal");
  filler.FillDecl<SolidTexturingVertex::TTexCoord>("a_colorTexCoords");

  return filler.m_info;
}

dp::BindingInfo MaskedTexturingBindingInit()
{
  static_assert(sizeof(MaskedTexturingVertex) ==
                    (sizeof(MaskedTexturingVertex::TPosition3d) + sizeof(MaskedTexturingVertex::TNormal) +
                     sizeof(MaskedTexturingVertex::TTexCoord) + sizeof(MaskedTexturingVertex::TTexCoord)),
                "");

  dp::BindingFiller<MaskedTexturingVertex> filler(4);
  filler.FillDecl<SolidTexturingVertex::TPosition3d>("a_position");
  filler.FillDecl<SolidTexturingVertex::TNormal>("a_normal");
  filler.FillDecl<SolidTexturingVertex::TTexCoord>("a_colorTexCoords");
  filler.FillDecl<SolidTexturingVertex::TTexCoord>("a_maskTexCoords");

  return filler.m_info;
}

dp::BindingInfo TextStaticBindingInit()
{
  static_assert(sizeof(TextStaticVertex) == (2 * sizeof(TextStaticVertex::TTexCoord)), "");

  dp::BindingFiller<TextStaticVertex> filler(2);
  filler.FillDecl<TextStaticVertex::TTexCoord>("a_colorTexCoord");
  filler.FillDecl<TextStaticVertex::TTexCoord>("a_maskTexCoord");

  return filler.m_info;
}

dp::BindingInfo TextOutlinedStaticBindingInit()
{
  static_assert(sizeof(TextOutlinedStaticVertex) == (3 * sizeof(TextOutlinedStaticVertex::TTexCoord)), "");

  dp::BindingFiller<TextOutlinedStaticVertex> filler(3);
  filler.FillDecl<TextOutlinedStaticVertex::TTexCoord>("a_colorTexCoord");
  filler.FillDecl<TextOutlinedStaticVertex::TTexCoord>("a_outlineColorTexCoord");
  filler.FillDecl<TextOutlinedStaticVertex::TTexCoord>("a_maskTexCoord");

  return filler.m_info;
}

dp::BindingInfo TextDynamicBindingInit()
{
  static_assert(
      sizeof(TextDynamicVertex) == (sizeof(TextStaticVertex::TPosition3d) + sizeof(TextDynamicVertex::TNormal)), "");

  dp::BindingFiller<TextDynamicVertex> filler(2, TextDynamicVertex::GetDynamicStreamID());
  filler.FillDecl<TextDynamicVertex::TPosition3d>("a_position");
  filler.FillDecl<TextDynamicVertex::TNormal>("a_normal");

  return filler.m_info;
}

dp::BindingInfo LineBindingInit()
{
  static_assert(
      sizeof(LineVertex) == sizeof(LineVertex::TPosition) + sizeof(LineVertex::TNormal) + sizeof(LineVertex::TTexCoord),
      "");
  dp::BindingFiller<LineVertex> filler(3);
  filler.FillDecl<LineVertex::TPosition>("a_position");
  filler.FillDecl<LineVertex::TNormal>("a_normal");
  filler.FillDecl<LineVertex::TTexCoord>("a_colorTexCoord");

  return filler.m_info;
}

dp::BindingInfo DashedLineBindingInit()
{
  static_assert(sizeof(DashedLineVertex) == sizeof(DashedLineVertex::TPosition) + sizeof(DashedLineVertex::TNormal) +
                                                sizeof(DashedLineVertex::TTexCoord) +
                                                sizeof(DashedLineVertex::TMaskTexCoord),
                "");

  dp::BindingFiller<DashedLineVertex> filler(4);
  filler.FillDecl<DashedLineVertex::TPosition>("a_position");
  filler.FillDecl<DashedLineVertex::TNormal>("a_normal");
  filler.FillDecl<DashedLineVertex::TTexCoord>("a_colorTexCoord");
  filler.FillDecl<DashedLineVertex::TMaskTexCoord>("a_maskTexCoord");

  return filler.m_info;
}

dp::BindingInfo RouteBindingInit()
{
  static_assert(sizeof(RouteVertex) == sizeof(RouteVertex::TPosition) + sizeof(RouteVertex::TNormal) +
                                           sizeof(RouteVertex::TLength) + sizeof(RouteVertex::TColor),
                "");

  dp::BindingFiller<RouteVertex> filler(4);
  filler.FillDecl<RouteVertex::TPosition>("a_position");
  filler.FillDecl<RouteVertex::TNormal>("a_normal");
  filler.FillDecl<RouteVertex::TLength>("a_length");
  filler.FillDecl<RouteVertex::TColor>("a_color");

  return filler.m_info;
}

dp::BindingInfo RouteMarkerBindingInit()
{
  static_assert(sizeof(RouteMarkerVertex) == sizeof(RouteMarkerVertex::TPosition) + sizeof(RouteMarkerVertex::TNormal) +
                                                 sizeof(RouteMarkerVertex::TColor),
                "");

  dp::BindingFiller<RouteMarkerVertex> filler(3);
  filler.FillDecl<RouteMarkerVertex::TPosition>("a_position");
  filler.FillDecl<RouteMarkerVertex::TNormal>("a_normal");
  filler.FillDecl<RouteMarkerVertex::TColor>("a_color");

  return filler.m_info;
}

dp::BindingInfo ColoredSymbolBindingInit()
{
  static_assert(sizeof(ColoredSymbolVertex) == sizeof(ColoredSymbolVertex::TPosition) +
                                                   sizeof(ColoredSymbolVertex::TNormal) +
                                                   sizeof(ColoredSymbolVertex::TTexCoord),
                "");

  dp::BindingFiller<ColoredSymbolVertex> filler(3);
  filler.FillDecl<ColoredSymbolVertex::TPosition>("a_position");
  filler.FillDecl<ColoredSymbolVertex::TNormal>("a_normal");
  filler.FillDecl<ColoredSymbolVertex::TTexCoord>("a_colorTexCoords");

  return filler.m_info;
}

BindingNode g_bindingNodes[TypeCount];
TInitFunction g_initFunctions[TypeCount] = {&AreaBindingInit,
                                            &Area3dBindingInit,
                                            &HatchingAreaBindingInit,
                                            &SolidTexturingBindingInit,
                                            &MaskedTexturingBindingInit,
                                            &TextStaticBindingInit,
                                            &TextOutlinedStaticBindingInit,
                                            &TextDynamicBindingInit,
                                            &LineBindingInit,
                                            &DashedLineBindingInit,
                                            &RouteBindingInit,
                                            &RouteMarkerBindingInit,
                                            &ColoredSymbolBindingInit};

dp::BindingInfo const & GetBinding(VertexType type)
{
  BindingNode & node = g_bindingNodes[type];
  if (!node.m_inited)
  {
    node.m_info = g_initFunctions[type]();
    node.m_inited = true;
  }

  return node.m_info;
}
}  // namespace

AreaVertex::AreaVertex(TPosition const & position, TTexCoord const & colorTexCoord)
  : m_position(position)
  , m_colorTexCoord(colorTexCoord)
{}

dp::BindingInfo const & AreaVertex::GetBindingInfo()
{
  return GetBinding(Area);
}

Area3dVertex::Area3dVertex(TPosition const & position, TPosition const & normal, TTexCoord const & colorTexCoord)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(colorTexCoord)
{}

dp::BindingInfo const & Area3dVertex::GetBindingInfo()
{
  return GetBinding(Area3d);
}

HatchingAreaVertex::HatchingAreaVertex(TPosition const & position, TTexCoord const & colorTexCoord,
                                       TMaskTexCoord const & maskTexCoord)
  : m_position(position)
  , m_colorTexCoord(colorTexCoord)
  , m_maskTexCoord(maskTexCoord)
{}

dp::BindingInfo const & HatchingAreaVertex::GetBindingInfo()
{
  return GetBinding(HatchingArea);
}

SolidTexturingVertex::SolidTexturingVertex(TPosition3d const & position, TNormal const & normal,
                                           TTexCoord const & colorTexCoord)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(colorTexCoord)
{}

dp::BindingInfo const & SolidTexturingVertex::GetBindingInfo()
{
  return GetBinding(SolidTexturing);
}

MaskedTexturingVertex::MaskedTexturingVertex(TPosition3d const & position, TNormal const & normal,
                                             TTexCoord const & colorTexCoord, TTexCoord const & maskTexCoord)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(colorTexCoord)
  , m_maskTexCoord(maskTexCoord)
{}

dp::BindingInfo const & MaskedTexturingVertex::GetBindingInfo()
{
  return GetBinding(MaskedTexturing);
}

TextOutlinedStaticVertex::TextOutlinedStaticVertex(TTexCoord const & colorTexCoord, TTexCoord const & outlineTexCoord,
                                                   TTexCoord const & maskTexCoord)
  : m_colorTexCoord(colorTexCoord)
  , m_outlineTexCoord(outlineTexCoord)
  , m_maskTexCoord(maskTexCoord)
{}

dp::BindingInfo const & TextOutlinedStaticVertex::GetBindingInfo()
{
  return GetBinding(TextOutlinedStatic);
}

TextDynamicVertex::TextDynamicVertex(TPosition3d const & position, TNormal const & normal)
  : m_position(position)
  , m_normal(normal)
{}

dp::BindingInfo const & TextDynamicVertex::GetBindingInfo()
{
  return GetBinding(TextDynamic);
}

uint32_t TextDynamicVertex::GetDynamicStreamID()
{
  return 0x7F;
}

LineVertex::LineVertex(TPosition const & position, TNormal const & normal, TTexCoord const & color)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(color)
{}

dp::BindingInfo const & LineVertex::GetBindingInfo()
{
  return GetBinding(Line);
}

DashedLineVertex::DashedLineVertex(TPosition const & position, TNormal const & normal, TTexCoord const & color,
                                   TMaskTexCoord const & mask)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(color)
  , m_maskTexCoord(mask)
{}

dp::BindingInfo const & DashedLineVertex::GetBindingInfo()
{
  return GetBinding(DashedLine);
}

RouteVertex::RouteVertex(TPosition const & position, TNormal const & normal, TLength const & length,
                         TColor const & color)
  : m_position(position)
  , m_normal(normal)
  , m_length(length)
  , m_color(color)
{}

dp::BindingInfo const & RouteVertex::GetBindingInfo()
{
  return GetBinding(Route);
}

RouteMarkerVertex::RouteMarkerVertex(TPosition const & position, TNormal const & normal, TColor const & color)
  : m_position(position)
  , m_normal(normal)
  , m_color(color)
{}

dp::BindingInfo const & RouteMarkerVertex::GetBindingInfo()
{
  return GetBinding(RouteMarker);
}

TextStaticVertex::TextStaticVertex(TTexCoord const & colorTexCoord, TTexCoord const & maskTexCoord)
  : m_colorTexCoord(colorTexCoord)
  , m_maskTexCoord(maskTexCoord)
{}

dp::BindingInfo const & TextStaticVertex::GetBindingInfo()
{
  return GetBinding(TextStatic);
}

ColoredSymbolVertex::ColoredSymbolVertex(TPosition const & position, TNormal const & normal,
                                         TTexCoord const & colorTexCoord)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(colorTexCoord)
{}

dp::BindingInfo const & ColoredSymbolVertex::GetBindingInfo()
{
  return GetBinding(ColoredSymbol);
}
}  // namespace gpu
