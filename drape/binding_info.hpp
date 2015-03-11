#pragma once

#include "drape/glfunctions.hpp"

#include "std/string.hpp"
#include "std/shared_array.hpp"

namespace dp
{

struct BindingDecl
{
  string  m_attributeName;
  uint8_t m_componentCount;
  glConst m_componentType;
  uint8_t m_stride;
  uint8_t m_offset;

  bool operator != (BindingDecl const & other) const;
  bool operator < (BindingDecl const & other) const;
};

class BindingInfo
{
public:
  BindingInfo();
  BindingInfo(uint8_t count, uint8_t id = 0);
  ~BindingInfo();

  uint8_t GetCount() const;
  uint8_t GetID() const;
  BindingDecl const & GetBindingDecl(uint16_t index) const;
  BindingDecl & GetBindingDecl(uint16_t index);
  uint16_t GetElementSize() const;
  bool IsDynamic() const;

  bool operator< (BindingInfo const & other) const;

protected:
  shared_array<BindingDecl> m_bindings;
  uint16_t m_info;
};

} // namespace dp
