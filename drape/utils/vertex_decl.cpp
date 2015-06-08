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

  dp::BindingInfo info(3);

  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = glsl::GetComponentCount<SolidTexturingVertex::TPosition>();
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = sizeof(SolidTexturingVertex);

  dp::BindingDecl & normalDecl = info.GetBindingDecl(1);
  normalDecl.m_attributeName = "a_normal";
  normalDecl.m_componentCount = glsl::GetComponentCount<SolidTexturingVertex::TNormal>();
  normalDecl.m_componentType = gl_const::GLFloatType;
  normalDecl.m_offset = sizeof(SolidTexturingVertex::TPosition);
  normalDecl.m_stride = posDecl.m_stride;

  dp::BindingDecl & colorTexCoordDecl = info.GetBindingDecl(2);
  colorTexCoordDecl.m_attributeName = "a_colorTexCoords";
  colorTexCoordDecl.m_componentCount = glsl::GetComponentCount<SolidTexturingVertex::TTexCoord>();
  colorTexCoordDecl.m_componentType = gl_const::GLFloatType;
  colorTexCoordDecl.m_offset = normalDecl.m_offset + sizeof(SolidTexturingVertex::TNormal);
  colorTexCoordDecl.m_stride = posDecl.m_stride;

  return info;
}

dp::BindingInfo TextStaticBindingInit()
{
  static_assert(sizeof(TextStaticVertex) == (sizeof(TextStaticVertex::TPosition) +
                                             3 * sizeof(TextStaticVertex::TTexCoord)), "");
  dp::BindingInfo info(4);

  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = glsl::GetComponentCount<TextStaticVertex::TPosition>();
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = sizeof(TextStaticVertex);

  dp::BindingDecl & colorDecl = info.GetBindingDecl(1);
  colorDecl.m_attributeName = "a_colorTexCoord";
  colorDecl.m_componentCount = glsl::GetComponentCount<TextStaticVertex::TTexCoord>();
  colorDecl.m_componentType = gl_const::GLFloatType;
  colorDecl.m_offset = sizeof(TextStaticVertex::TPosition);
  colorDecl.m_stride = posDecl.m_stride;

  dp::BindingDecl & outlineDecl = info.GetBindingDecl(2);
  outlineDecl.m_attributeName = "a_outlineColorTexCoord";
  outlineDecl.m_componentCount = glsl::GetComponentCount<TextStaticVertex::TTexCoord>();
  outlineDecl.m_componentType = gl_const::GLFloatType;
  outlineDecl.m_offset = colorDecl.m_offset + sizeof(TextStaticVertex::TTexCoord);
  outlineDecl.m_stride = posDecl.m_stride;

  dp::BindingDecl & maskDecl = info.GetBindingDecl(3);
  maskDecl.m_attributeName = "a_maskTexCoord";
  maskDecl.m_componentCount = glsl::GetComponentCount<TextStaticVertex::TTexCoord>();
  maskDecl.m_componentType = gl_const::GLFloatType;
  maskDecl.m_offset = outlineDecl.m_offset + sizeof(TextStaticVertex::TTexCoord);
  maskDecl.m_stride = posDecl.m_stride;

  return info;
}

dp::BindingInfo TextDynamicBindingInit()
{
  static_assert(sizeof(TextDynamicVertex) == sizeof(TextDynamicVertex::TNormal), "");
  dp::BindingInfo info(1, TextDynamicVertex::GetDynamicStreamID());

  dp::BindingDecl & decl = info.GetBindingDecl(0);
  decl.m_attributeName = "a_normal";
  decl.m_componentCount = glsl::GetComponentCount<TextDynamicVertex::TNormal>();
  decl.m_componentType = gl_const::GLFloatType;
  decl.m_offset = 0;
  decl.m_stride = sizeof(TextDynamicVertex);

  return info;
}

dp::BindingInfo LineBindingInit()
{
  static_assert(sizeof(LineVertex) == sizeof(LineVertex::TPosition) +
                                      sizeof(LineVertex::TNormal) +
                                      2 * sizeof(LineVertex::TTexCoord), "");
  dp::BindingInfo info(4);
>

  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = glsl::GetComponentCount<LineVertex::TPosition>();
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = sizeof(LineVertex);

  dp::BindingDecl & normalDecl = info.GetBindingDecl(1);
  normalDecl.m_attributeName = "a_normal";
  normalDecl.m_componentCount = glsl::GetComponentCount<LineVertex::TNormal>();
  normalDecl.m_componentType = gl_const::GLFloatType;
  normalDecl.m_offset = posDecl.m_offset + sizeof(LineVertex::TPosition);
  normalDecl.m_stride = posDecl.m_stride;

  dp::BindingDecl & colorDecl = info.GetBindingDecl(2);
  colorDecl.m_attributeName = "a_colorTexCoord";
  colorDecl.m_componentCount = glsl::GetComponentCount<LineVertex::TTexCoord>();
  colorDecl.m_componentType = gl_const::GLFloatType;
  colorDecl.m_offset = normalDecl.m_offset + sizeof(LineVertex::TNormal);
  colorDecl.m_stride = posDecl.m_stride;

  dp::BindingDecl & maskDecl = info.GetBindingDecl(3);
  maskDecl.m_attributeName = "a_maskTexCoord";
  maskDecl.m_componentCount = glsl::GetComponentCount<LineVertex::TTexCoord>();
  maskDecl.m_componentType = gl_const::GLFloatType;
  maskDecl.m_offset = colorDecl.m_offset + sizeof(LineVertex::TTexCoord);
  maskDecl.m_stride = posDecl.m_stride;

  return info;
}

dp::BindingInfo RouteBindingInit()
{
  STATIC_ASSERT(sizeof(RouteVertex) == sizeof(RouteVertex::TPosition) +
                                       sizeof(RouteVertex::TNormal) +
                                       sizeof(RouteVertex::TLength));
  dp::BindingInfo info(3);

  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = glsl::GetComponentCount<RouteVertex::TPosition>();
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = sizeof(RouteVertex);

  dp::BindingDecl & normalDecl = info.GetBindingDecl(1);
  normalDecl.m_attributeName = "a_normal";
  normalDecl.m_componentCount = glsl::GetComponentCount<RouteVertex::TNormal>();
  normalDecl.m_componentType = gl_const::GLFloatType;
  normalDecl.m_offset = posDecl.m_offset + sizeof(RouteVertex::TPosition);
  normalDecl.m_stride = posDecl.m_stride;

  dp::BindingDecl & lengthDecl = info.GetBindingDecl(2);
  lengthDecl.m_attributeName = "a_length";
  lengthDecl.m_componentCount = glsl::GetComponentCount<RouteVertex::TLength>();
  lengthDecl.m_componentType = gl_const::GLFloatType;
  lengthDecl.m_offset = normalDecl.m_offset + sizeof(RouteVertex::TNormal);
  lengthDecl.m_stride = posDecl.m_stride;

  return info;
}

BindingNode g_bindingNodes[TypeCount];
TInitFunction g_initFunctions[TypeCount] =
{
  &SolidTexturingBindingInit,
  &TextStaticBindingInit,
  &TextDynamicBindingInit,
  &LineBindingInit,
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
  , m_normal(0.0, 0.0)
  , m_colorTexCoord(0.0, 0.0)
  , m_maskTexCoord(0.0, 0.0)
{
}

LineVertex::LineVertex(TPosition const & position, TNormal const & normal,
                       TTexCoord const & color, TTexCoord const & mask)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(color)
  , m_maskTexCoord(mask)
{
}

dp::BindingInfo const & LineVertex::GetBindingInfo()
{
  return GetBinding(Line);
}

RouteVertex::RouteVertex()
  : m_position(0.0, 0.0, 0.0)
  , m_normal(0.0, 0.0)
  , m_length(0.0, 0.0)
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

