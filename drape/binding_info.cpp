#include "binding_info.hpp"

#include "../base/assert.hpp"

namespace
{

#include "glIncludes.hpp"
uint16_t sizeOfType(glConst type)
{
  if (type == gl_const::GLByteType || type == gl_const::GLUnsignedByteType)
    return sizeof(GLbyte);
  else if (type == gl_const::GLShortType || type == gl_const::GLUnsignedShortType)
    return sizeof(GLshort);
  else if (type == gl_const::GLIntType || type == gl_const::GLUnsignedIntType)
    return sizeof(GLint);
  else if (type == gl_const::GLFloatType)
    return sizeof(GLfloat);
  else if (type == gl_const::GLDoubleType)
    return sizeof(GLdouble);

  ASSERT(false, ());
  return 0;
}

} // namespace

bool BindingDecl::operator!=(BindingDecl const & other) const
{
  return m_attributeName != other.m_attributeName ||
      m_componentCount != other.m_componentCount  ||
      m_componentType != other.m_componentType    ||
      m_stride != other.m_stride                  ||
      m_offset != other.m_offset;
}

bool BindingDecl::operator<(BindingDecl const & other) const
{
  if (m_attributeName != other.m_attributeName)
    return m_attributeName < other.m_attributeName;
  if (m_componentCount != other.m_componentCount)
    return m_componentCount < other.m_componentCount;
  if (m_componentType != other.m_componentType)
    return m_componentType < other.m_componentType;
  if (m_stride != other.m_stride)
    return m_stride < other.m_stride;
  return m_offset < other.m_offset;
}

BindingInfo::BindingInfo()
{
  m_size = 0;
}

BindingInfo::BindingInfo(uint16_t count)
{
  m_bindings.reset(new BindingDecl[count]);
  m_size = count;
}

BindingInfo::~BindingInfo()
{
}

uint16_t BindingInfo::GetCount() const
{
  return m_size;
}

BindingDecl const & BindingInfo::GetBindingDecl(uint16_t index) const
{
  ASSERT_LESS(index, m_size, ());
  return m_bindings[index];
}

BindingDecl & BindingInfo::GetBindingDecl(uint16_t index)
{
  ASSERT_LESS(index, m_size, ());
  return m_bindings[index];
}

uint16_t BindingInfo::GetElementSize() const
{
  if (m_size == 0)
    return 0;

  uint8_t stride = m_bindings[0].m_stride;
  if (stride != 0)
    return stride;

  int calcStride = 0;
  for (uint16_t i = 0; i < m_size; ++i)
    calcStride += (m_bindings[i].m_componentCount * sizeOfType(m_bindings[i].m_componentType));

  return calcStride;
}

bool BindingInfo::operator<(BindingInfo const & other) const
{
  if (m_size != other.m_size)
    return m_size < other.m_size;

  for (uint16_t i = 0; i < m_size; ++i)
  {
    BindingDecl & thisDecl = m_bindings[i];
    BindingDecl & otherDecl = other.m_bindings[i];
    if (thisDecl != otherDecl)
      return thisDecl < otherDecl;
  }

  return false;
}
