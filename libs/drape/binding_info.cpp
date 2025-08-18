#include "drape/binding_info.hpp"
#include "drape/gl_includes.hpp"

#include "base/assert.hpp"

namespace dp
{
namespace
{
uint16_t SizeOfType(glConst type)
{
  if (type == gl_const::GLByteType || type == gl_const::GLUnsignedByteType)
    return sizeof(GLbyte);
  else if (type == gl_const::GLShortType || type == gl_const::GLUnsignedShortType)
    return sizeof(GLshort);
  else if (type == gl_const::GLIntType || type == gl_const::GLUnsignedIntType)
    return sizeof(GLint);
  else if (type == gl_const::GLFloatType)
    return sizeof(GLfloat);

  ASSERT(false, ());
  return 0;
}
}  // namespace

bool BindingDecl::operator==(BindingDecl const & other) const
{
  return m_attributeName == other.m_attributeName && m_componentCount == other.m_componentCount &&
         m_componentType == other.m_componentType && m_stride == other.m_stride && m_offset == other.m_offset;
}

bool BindingDecl::operator!=(BindingDecl const & other) const
{
  return !operator==(other);
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

BindingInfo::BindingInfo() : m_info(0) {}

BindingInfo::BindingInfo(uint8_t count, uint8_t id) : m_info((static_cast<uint16_t>(count) << 8) | id)
{
  CHECK_LESS_OR_EQUAL(count, kMaxBindingDecl, ());
}

uint8_t BindingInfo::GetCount() const
{
  return static_cast<uint8_t>((m_info & 0xFF00) >> 8);
}

uint8_t BindingInfo::GetID() const
{
  return static_cast<uint8_t>(m_info & 0xFF);
}

BindingDecl const & BindingInfo::GetBindingDecl(uint16_t index) const
{
  ASSERT_LESS(index, GetCount(), ());
  return m_bindings[index];
}

BindingDecl & BindingInfo::GetBindingDecl(uint16_t index)
{
  ASSERT_LESS(index, GetCount(), ());
  return m_bindings[index];
}

uint16_t BindingInfo::GetElementSize() const
{
  if (GetCount() == 0)
    return 0;

  uint8_t stride = m_bindings[0].m_stride;
  if (stride != 0)
    return stride;

  int calcStride = 0;
  for (uint16_t i = 0; i < GetCount(); ++i)
    calcStride += (m_bindings[i].m_componentCount * SizeOfType(m_bindings[i].m_componentType));

  return static_cast<uint16_t>(calcStride);
}

bool BindingInfo::IsDynamic() const
{
  return GetID() > 0;
}

bool BindingInfo::operator==(BindingInfo const & other) const
{
  if (m_info != other.m_info)
    return false;

  for (uint16_t i = 0; i < GetCount(); ++i)
  {
    BindingDecl const & thisDecl = m_bindings[i];
    BindingDecl const & otherDecl = other.m_bindings[i];
    if (thisDecl != otherDecl)
      return false;
  }
  return true;
}

bool BindingInfo::operator!=(BindingInfo const & other) const
{
  return !operator==(other);
}

bool BindingInfo::operator<(BindingInfo const & other) const
{
  if (m_info != other.m_info)
    return m_info < other.m_info;

  for (uint16_t i = 0; i < GetCount(); ++i)
  {
    BindingDecl const & thisDecl = m_bindings[i];
    BindingDecl const & otherDecl = other.m_bindings[i];
    if (thisDecl != otherDecl)
      return thisDecl < otherDecl;
  }
  return false;
}
}  // namespace dp
