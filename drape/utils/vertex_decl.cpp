#include "vertex_decl.hpp"

namespace gpu
{

SolidTexturingVertex::SolidTexturingVertex()
  : m_position(0.0, 0.0, 0.0)
  , m_normal(0.0, 0.0)
  , m_colorTexCoord(0.0, 0.0)
{
}

SolidTexturingVertex::SolidTexturingVertex(glm::vec3 const & position, glm::vec2 const & normal, glm::vec2 const & colorTexCoord)
  : m_position(position)
  , m_normal(normal)
  , m_colorTexCoord(colorTexCoord)
{
}

dp::BindingInfo const & SolidTexturingVertex::GetBindingInfo()
{
  STATIC_ASSERT(sizeof(SolidTexturingVertex) == (sizeof(glsl::vec3) + 2 * sizeof(glsl::vec2)));

  static dp::BindingInfo s_info(3);
  static bool s_inited = false;

  if (!s_inited)
  {
    s_inited = true;
    dp::BindingDecl & posDecl = s_info.GetBindingDecl(0);
    posDecl.m_attributeName = "a_position";
    posDecl.m_componentCount = 3;
    posDecl.m_componentType = gl_const::GLFloatType;
    posDecl.m_offset = 0;
    posDecl.m_stride = sizeof(SolidTexturingVertex);

    dp::BindingDecl & normalDecl = s_info.GetBindingDecl(1);
    normalDecl.m_attributeName = "a_normal";
    normalDecl.m_componentCount = 2;
    normalDecl.m_componentType = gl_const::GLFloatType;
    normalDecl.m_offset = sizeof(glsl::vec3);
    normalDecl.m_stride = posDecl.m_stride;

    dp::BindingDecl & colorTexCoordDecl = s_info.GetBindingDecl(2);
    colorTexCoordDecl.m_attributeName = "a_colorTexCoords";
    colorTexCoordDecl.m_componentCount = 2;
    colorTexCoordDecl.m_componentType = gl_const::GLFloatType;
    colorTexCoordDecl.m_offset = sizeof(glsl::vec3) + sizeof(glsl::vec2);
    colorTexCoordDecl.m_stride = posDecl.m_stride;
  }

  return s_info;
}

dp::BindingInfo const & TextStaticVertex::GetBindingInfo()
{
  STATIC_ASSERT(sizeof(TextStaticVertex) == (sizeof(glsl::vec3) + 3 * sizeof(glsl::vec2)));
  static dp::BindingInfo s_info(4);
  static bool s_inited = false;

  if (!s_inited)
  {
    s_inited = true;

    dp::BindingDecl & posDecl = s_info.GetBindingDecl(0);
    posDecl.m_attributeName = "a_position";
    posDecl.m_componentCount = 3;
    posDecl.m_componentType = gl_const::GLFloatType;
    posDecl.m_offset = 0;
    posDecl.m_stride = sizeof(TextStaticVertex);

    dp::BindingDecl & colorDecl = s_info.GetBindingDecl(1);
    colorDecl.m_attributeName = "a_colorTexCoord";
    colorDecl.m_componentCount = 2;
    colorDecl.m_componentType = gl_const::GLFloatType;
    colorDecl.m_offset = sizeof(glsl::vec3);
    colorDecl.m_stride = posDecl.m_stride;

    dp::BindingDecl & outlineDecl = s_info.GetBindingDecl(2);
    outlineDecl.m_attributeName = "a_outlineColorTexCoord";
    outlineDecl.m_componentCount = 2;
    outlineDecl.m_componentType = gl_const::GLFloatType;
    outlineDecl.m_offset = colorDecl.m_offset + sizeof(glsl::vec2);
    outlineDecl.m_stride = posDecl.m_stride;

    dp::BindingDecl & maskDecl = s_info.GetBindingDecl(3);
    maskDecl.m_attributeName = "a_maskTexCoord";
    maskDecl.m_componentCount = 2;
    maskDecl.m_componentType = gl_const::GLFloatType;
    maskDecl.m_offset = outlineDecl.m_offset + sizeof(glsl::vec2);
    maskDecl.m_stride = posDecl.m_stride;
  }

  return s_info;
}

dp::BindingInfo const & TextDynamicVertex::GetBindingInfo()
{
  STATIC_ASSERT(sizeof(TextDynamicVertex) == sizeof(glsl::vec2));
  static dp::BindingInfo s_info(1, GetDynamicStreamID());
  static bool s_inited = false;

  if (!s_inited)
  {
    dp::BindingDecl & decl = s_info.GetBindingDecl(0);
    decl.m_attributeName = "a_normal";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = sizeof(TextDynamicVertex);
  }

  return s_info;
}

uint32_t TextDynamicVertex::GetDynamicStreamID()
{
  return 0x7F;
}

} //namespace gpu

