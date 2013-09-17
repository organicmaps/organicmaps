#include "attribute_provider.hpp"
#include "../base/assert.hpp"

#ifdef DEBUG
  #define INIT_CHECK_INFO(x) m_checkInfo = vector<bool>((vector<bool>::size_type)(x), false);
  #define CHECK_STREAMS CheckStreams()
  #define INIT_STREAM(x) InitStream((x))
#else
  #include "macros.hpp"
  #define INIT_CHECK_INFO(x) UNUSED_VALUE((x))
  #define CHECK_STREAMS
  #define INIT_STREAM(x) UNUSED_VALUE((x))
#endif

AttributeProvider::AttributeProvider(uint8_t streamCount, uint16_t vertexCount)
  : m_vertexCount(vertexCount)
{
  m_streams.resize(streamCount);
  INIT_CHECK_INFO(streamCount);
}

/// interface for batcher
bool AttributeProvider::IsDataExists() const
{
  CHECK_STREAMS;
  return m_vertexCount > 0;
}

uint16_t AttributeProvider::GetVertexCount() const
{
  CHECK_STREAMS;
  return m_vertexCount;
}

uint8_t AttributeProvider::GetStreamCount() const
{
  return m_streams.size();
}

const void * AttributeProvider::GetRawPointer(uint8_t streamIndex)
{
  ASSERT(streamIndex < GetStreamCount(), ("Stream index = ", streamIndex, " out of range [0 : ", GetStreamCount(), ")"));
  CHECK_STREAMS;
  return m_streams[streamIndex].m_data.GetRaw();
}

const BindingInfo & AttributeProvider::GetBindingInfo(uint8_t streamIndex) const
{
  ASSERT(streamIndex < GetStreamCount(), ("Stream index = ", streamIndex, " out of range [0 : ", GetStreamCount(), ")"));
  CHECK_STREAMS;
  return m_streams[streamIndex].m_binding;
}

void AttributeProvider::Advance(uint16_t vertexCount)
{
  assert(m_vertexCount >= vertexCount);
  CHECK_STREAMS;
  for (size_t i = 0; i < GetStreamCount(); ++i)
  {
    const BindingInfo & info = m_streams[i].m_binding;
    uint32_t offset = vertexCount * info.GetElementSize();
    void * rawPointer = m_streams[i].m_data.GetRaw();
    m_streams[i].m_data = WeakPointer<void>((void *)(((uint8_t *)rawPointer) + offset));
  }

  m_vertexCount -= vertexCount;
}

void AttributeProvider::InitStream(uint8_t streamIndex,
                                   const BindingInfo &bindingInfo,
                                   WeakPointer<void> data)
{
  ASSERT(streamIndex < GetStreamCount(), ("Stream index = ", streamIndex, " out of range [0 : ", GetStreamCount(), ")"));
  AttributeStream s;
  s.m_binding = bindingInfo;
  s.m_data = data;
  m_streams[streamIndex] = s;
  INIT_STREAM(streamIndex);
}

#ifdef DEBUG
void AttributeProvider::CheckStreams() const
{
  ASSERT(std::find(m_checkInfo.begin(), m_checkInfo.end(), false) == m_checkInfo.end(),
         ("Not all streams initialized"));
}

void AttributeProvider::InitStream(uint8_t streamIndex)
{
  m_checkInfo[streamIndex] = true;
}
#endif
