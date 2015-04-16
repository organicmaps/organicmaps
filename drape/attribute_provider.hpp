#pragma once

#include "drape/pointers.hpp"
#include "drape/binding_info.hpp"

#include "std/vector.hpp"

namespace dp
{

class AttributeProvider
{
public:
  AttributeProvider(uint8_t streamCount, uint16_t vertexCount);

  /// interface for batcher
  bool IsDataExists() const;
  uint16_t GetVertexCount() const;

  uint8_t GetStreamCount() const;
  void const * GetRawPointer(uint8_t streamIndex);
  BindingInfo const & GetBindingInfo(uint8_t streamIndex) const;

  void Advance(uint16_t vertexCount);

  void InitStream(uint8_t streamIndex,
                  BindingInfo const & bindingInfo,
                  ref_ptr<void> data);

private:
  int32_t m_vertexCount;

  struct AttributeStream
  {
    BindingInfo m_binding;
    ref_ptr<void> m_data;
  };
  vector<AttributeStream> m_streams;
#ifdef DEBUG
  void CheckStreams() const;
  void InitCheckStream(uint8_t streamIndex);
  vector<bool> m_checkInfo;
#endif
};

} // namespace dp
