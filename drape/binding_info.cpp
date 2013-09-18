#include "binding_info.hpp"

#include "../base/assert.hpp"

namespace
{
  #include "glIncludes.hpp"
  uint16_t sizeOfType(glConst type)
  {
    if (type == GLConst::GLByteType || type == GLConst::GLUnsignedByteType)
      return sizeof(GLbyte);
    else if (type == GLConst::GLShortType || type == GLConst::GLUnsignedShortType)
      return sizeof(GLshort);
    else if (type == GLConst::GLIntType || type == GLConst::GLUnsignedIntType)
      return sizeof(GLint);
    else if (type == GLConst::GLFloatType)
      return sizeof(GLfloat);
    else if (type == GLConst::GLDoubleType)
      return sizeof(GLdouble);

    ASSERT(false, ());
    return 0;
  }
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

const BindingDecl & BindingInfo::GetBindingDecl(uint16_t index) const
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

bool BindingInfo::operator< (const BindingInfo & other) const
{
  return this < &other;
}
