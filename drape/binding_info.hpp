#pragma once

#include "drape/gl_constants.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace dp
{
size_t constexpr kMaxBindingDecl = 8;
size_t constexpr kMaxBindingInfo = 3;

struct BindingDecl
{
  std::string m_attributeName;
  uint8_t m_componentCount;
  glConst m_componentType;
  uint8_t m_stride;
  uint8_t m_offset;

  bool operator==(BindingDecl const & other) const;
  bool operator!=(BindingDecl const & other) const;
  bool operator<(BindingDecl const & other) const;
};

class BindingInfo
{
public:
  BindingInfo();
  explicit BindingInfo(uint8_t count, uint8_t id = 0);

  uint8_t GetCount() const;
  uint8_t GetID() const;
  BindingDecl const & GetBindingDecl(uint16_t index) const;
  BindingDecl & GetBindingDecl(uint16_t index);
  uint16_t GetElementSize() const;
  bool IsDynamic() const;

  bool operator==(BindingInfo const & other) const;
  bool operator!=(BindingInfo const & other) const;
  bool operator<(BindingInfo const & other) const;

protected:
  std::array<BindingDecl, kMaxBindingDecl> m_bindings;
  uint16_t m_info;
};

template <typename TFieldType, typename TVertexType>
uint8_t FillDecl(size_t index, std::string const & attrName, dp::BindingInfo & info, uint8_t offset)
{
  dp::BindingDecl & decl = info.GetBindingDecl(static_cast<uint16_t>(index));
  decl.m_attributeName = attrName;
  decl.m_componentCount = glsl::GetComponentCount<TFieldType>();
  decl.m_componentType = gl_const::GLFloatType;
  decl.m_offset = offset;
  decl.m_stride = sizeof(TVertexType);

  return sizeof(TFieldType);
}

using BindingInfoArray = std::array<dp::BindingInfo, kMaxBindingInfo>;

template <typename TVertex>
class BindingFiller
{
public:
  explicit BindingFiller(uint8_t count, uint8_t id = 0) : m_info(count, id) {}

  template <typename TFieldType>
  void FillDecl(std::string const & attrName)
  {
    m_offset += dp::FillDecl<TFieldType, TVertex>(m_index, attrName, m_info, m_offset);
    ++m_index;
  }

  dp::BindingInfo m_info;

private:
  size_t m_index = 0;
  uint8_t m_offset = 0;
};
}  // namespace dp
