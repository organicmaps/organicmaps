#include "drape/utils/vertex_decl.hpp"

namespace gpu
{

namespace
{

enum VertexType
{
  SolidTexturing,
  TextStatic,
  TextDynamic,
  Line,
  DashedLine,
  Route,
  TypeCount
};

struct BindingNode
{
  dp::BindingInfo m_info;
  bool m_inited = false;
};

typedef dp::BindingInfo (*TInitFunction)();

dp::BindingInfo SolidTexturingBindingInit()
{
  static_assert(sizeof(SolidTexturingVertex) == (sizeof(SolidTexturingVertex::TPosition) +
                                                 sizeof(SolidTexturingVertex::TNormal) +
                                                 sizeof(SolidTexturingVertex::TTexCoord)), "");

  dp::BindingFiller<SolidTexturingVertex> filler(3);
  filler.FillDecl<SolidTexturingVertex::TPosition>("a_position");
  filler.FillDecl<SolidTexturingVertex::TNormal>("a_normal");
  filler.FillDecl<SolidTexturingVertex::TTexCoord>("a_colorTexCoords");

  return filler.m_info;
}

dp::BindingInfo TextStaticBindingInit()
{
  static_assert(sizeof(TextStaticVertex) == (sizeof(TextStaticVertex::TPosition) +
                                             3 * sizeof(TextStaticVertex::TTexCoord)), "");

  dp::BindingFiller<TextStaticVertex> filler(4);
  filler.FillDecl<TextStaticVertex::TPosition>("a_position");
  filler.FillDecl<TextStaticVertex::TTexCoord>("a_colorTexCoord");
  filler.FillDecl<TextStaticVertex::TTexCoord>("a_outlineColorTexCoord");
  filler.FillDecl<TextStaticVertex::TTexCoord>("a_maskTexCoord");

  return filler.m_info;
}

dp::BindingInfo TextDynamicBindingInit()
{
  static_assert(sizeof(TextDynamicVertex) == sizeof(TextDynamicVertex::TNormal), "");

  dp::BindingFiller<TextDynamicVertex> filler(1, TextDynamicVertex::GetDynamicStreamID());
  filler.FillDecl<TextDynamicVertex::TNormal>("a_normal");

  return filler.m_info;
}

dp::BindingInfo LineBindingInit()
{
  static_assert(sizeof(LineVertex) == sizeof(LineVertex::TPosition) +
                                      sizeof(LineVertex::TNormal) +
                                      sizeof(LineVertex::TTexCoord), "");
  dp::BindingFiller<LineVertex> filler(3);
  filler.FillDecl<LineVertex::TPosition>("a_position");
  filler.FillDecl<LineVertex::TNormal>("a_normal");
  filler.FillDecl<LineVertex::TTexCoord>("a_colorTexCoord");

  return filler.m_info;
}

dp::BindingInfo DashedLineBindingInit()
{
  static_assert(sizeof(DashedLineVertex) == sizeof(DashedLineVertex::TPosition) +
                                            sizeof(DashedLineVertex::TNormal) +
                                            2 * sizeof(DashedLineVertex::TTexCoord), "");

  dp::BindingFiller<DashedLineVertex> filler(4);
  filler.FillDecl<DashedLineVertex::TPosition>("a_position");
  filler.FillDecl<DashedLineVertex::TNormal>("a_normal");
  filler.FillDecl<DashedLineVertex::TTexCoord>("a_colorTexCoord");
  filler.FillDecl<DashedLineVertex::TTexCoord>("a_maskTexCoord");

  return filler.m_info;
}

dp::BindingInfo RouteBindingInit()
{
  static_assert(sizeof(RouteVertex) == sizeof(RouteVertex::TPosition) +
                                       sizeof(RouteVertex::TNormal) +
                                       sizeof(RouteVertex::TLength), "");

  dp::BindingFiller<RouteVertex> filler(3);
  filler.FillDecl<RouteVertex::TPosition>("a_position");
  filler.FillDecl<RouteVertex::TNormal>("a_normal");
  filler.FillDecl<RouteVertex::TLength>("a_length");

  return filler.m_info;
}

BindingNode g_bindingNodes[TypeCount];
TInitFunction g_initFunctions[TypeCount] =
{
  &SolidTexturingBindingInit,
  &TextStaticBindingInit,
  &TextDynamicBindingInit,
  &LineBindingInit,
  &DashedLineBindingInit,
  &RouteBindingInit
};

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

} // namespace

SolidTexturingVertex::SolidTexturingVertex()
  : m_position(0.0, 0.0, 0.0)
  , m_normal(0.0, 0.0)
  , m_colorTexCoord(0.0, 0.0)
{
}

SolidTexturingVertex::SolidTexturingVertex(TPosition const & position, TNormal const & normal,
                                           TTexCoord const & colorTexCoord)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(colorTexCoord)
{
}

dp::BindingInfo const & SolidTexturingVertex::GetBindingInfo()
{
  return GetBinding(SolidTexturing);
}

TextStaticVertex::TextStaticVertex()
  : m_position(0.0, 0.0, 0.0)
  , m_colorTexCoord(0.0, 0.0)
  , m_outlineTexCoord(0.0, 0.0)
  , m_maskTexCoord(0.0, 0.0)
{
}

TextStaticVertex::TextStaticVertex(TPosition const & position, TTexCoord const & colorTexCoord,
                                   TTexCoord const & outlineTexCoord, TTexCoord const & maskTexCoord)
  : m_position(position)
  , m_colorTexCoord(colorTexCoord)
  , m_outlineTexCoord(outlineTexCoord)
  , m_maskTexCoord(maskTexCoord)
{
}

dp::BindingInfo const & TextStaticVertex::GetBindingInfo()
{
  return GetBinding(TextStatic);
}

TextDynamicVertex::TextDynamicVertex()
  : m_normal(0.0, 0.0)
{
}

TextDynamicVertex::TextDynamicVertex(TNormal const & normal)
  : m_normal(normal)
{
}

dp::BindingInfo const & TextDynamicVertex::GetBindingInfo()
{
  return GetBinding(TextDynamic);
}

uint32_t TextDynamicVertex::GetDynamicStreamID()
{
  return 0x7F;
}

LineVertex::LineVertex()
  : m_position(0.0, 0.0, 0.0)
  , m_normal(0.0, 0.0, 0.0)
  , m_colorTexCoord(0.0, 0.0)
{
}

LineVertex::LineVertex(TPosition const & position, TNormal const & normal, TTexCoord const & color)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(color)
{
}

dp::BindingInfo const & LineVertex::GetBindingInfo()
{
  return GetBinding(Line);
}

DashedLineVertex::DashedLineVertex()
  : m_maskTexCoord(0.0, 0.0)
{
}

DashedLineVertex::DashedLineVertex(TPosition const & position, TNormal const & normal,
                                   TTexCoord const & color, TTexCoord const & mask)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(color)
  , m_maskTexCoord(mask)
{
}

dp::BindingInfo const & DashedLineVertex::GetBindingInfo()
{
  return GetBinding(DashedLine);
}

RouteVertex::RouteVertex()
  : m_position(0.0, 0.0, 0.0)
  , m_normal(0.0, 0.0)
  , m_length(0.0, 0.0, 0.0)
{}

RouteVertex::RouteVertex(TPosition const & position, TNormal const & normal, TLength const & length)
  : m_position(position)
  , m_normal(normal)
  , m_length(length)
{}

dp::BindingInfo const & RouteVertex::GetBindingInfo()
{
  return GetBinding(Route);
}

} //namespace gpu

