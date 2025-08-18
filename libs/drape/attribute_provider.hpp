#pragma once

#include "drape/binding_info.hpp"
#include "drape/pointers.hpp"

#include <vector>

namespace dp
{
class AttributeProvider
{
public:
  AttributeProvider(uint8_t streamCount, uint32_t vertexCount);

  bool IsDataExists() const;
  uint32_t GetVertexCount() const;

  uint8_t GetStreamCount() const;
  void const * GetRawPointer(uint8_t streamIndex);
  BindingInfo const & GetBindingInfo(uint8_t streamIndex) const;

  void Advance(uint32_t vertexCount);

  void InitStream(uint8_t streamIndex, BindingInfo const & bindingInfo, ref_ptr<void> data);

  void Reset(uint32_t vertexCount);
  void UpdateStream(uint8_t streamIndex, ref_ptr<void> data);

private:
  uint32_t m_vertexCount;

  struct AttributeStream
  {
    BindingInfo m_binding;
    ref_ptr<void> m_data;
  };
  std::vector<AttributeStream> m_streams;
#ifdef DEBUG
  void CheckStreams() const;
  void InitCheckStream(uint8_t streamIndex);
  std::vector<bool> m_checkInfo;
#endif
};
}  // namespace dp
