#pragma once

#include "pointers.hpp"
#include "binding_info.hpp"

#include "../std/vector.hpp"

class AttributeProvider
{
public:
  AttributeProvider(uint8_t streamCount, uint16_t vertexCount);

  /// interface for batcher
  bool IsDataExists() const;
  uint16_t GetVertexCount() const;

  uint8_t GetStreamCount() const;
  const void * GetRawPointer(uint8_t streamIndex);
  const BindingInfo & GetBindingInfo(uint8_t streamIndex) const;

  void Advance(uint16_t vertexCount);

  void InitStream(uint8_t streamIndex,
                  const BindingInfo & bindingInfo,
                  RefPointer<void> data);

private:
  int32_t m_vertexCount;

  struct AttributeStream
  {
    BindingInfo m_binding;
    RefPointer<void> m_data;
  };
  vector<AttributeStream> m_streams;
#ifdef DEBUG
  void CheckStreams() const;
  void InitCheckStream(uint8_t streamIndex);
  vector<bool> m_checkInfo;
#endif
};
