#pragma once

#include "glfunctions.hpp"

#include "../std/string.hpp"
#include "../std/shared_array.hpp"

struct BindingDecl
{
  string    m_attributeName;
  uint8_t   m_componentCount;
  glConst   m_componentType;
  uint8_t   m_stride;
  uint16_t  m_offset;

  bool operator != (const BindingDecl & other) const;
  bool operator < (const BindingDecl & other) const;
};

class BindingInfo
{
public:
  BindingInfo();
  BindingInfo(uint16_t count);
  ~BindingInfo();

  uint16_t GetCount() const;
  const BindingDecl & GetBindingDecl(uint16_t index) const;
  BindingDecl & GetBindingDecl(uint16_t index);
  uint16_t GetElementSize() const;

  bool operator< (const BindingInfo & other) const;

protected:
  shared_array<BindingDecl> m_bindings;
  uint16_t m_size;
};
