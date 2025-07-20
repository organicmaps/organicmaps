#include "drape/attribute_provider.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#ifdef DEBUG
  #define INIT_CHECK_INFO(x) m_checkInfo = std::vector<bool>((std::vector<bool>::size_type)(x), false);
  #define CHECK_STREAMS CheckStreams()
  #define INIT_STREAM(x) InitCheckStream((x))
#else
  #include "base/macros.hpp"
  #define INIT_CHECK_INFO(x) UNUSED_VALUE((x))
  #define CHECK_STREAMS
  #define INIT_STREAM(x) UNUSED_VALUE((x))
#endif

namespace dp
{
AttributeProvider::AttributeProvider(uint8_t streamCount, uint32_t vertexCount)
  : m_vertexCount(vertexCount)
{
  m_streams.resize(streamCount);
  INIT_CHECK_INFO(streamCount);
}

bool AttributeProvider::IsDataExists() const
{
  CHECK_STREAMS;
  return m_vertexCount > 0;
}

uint32_t AttributeProvider::GetVertexCount() const
{
  return m_vertexCount;
}

uint8_t AttributeProvider::GetStreamCount() const
{
  return m_streams.size();
}

void const * AttributeProvider::GetRawPointer(uint8_t streamIndex)
{
  ASSERT_LESS(streamIndex, GetStreamCount(), ());
  CHECK_STREAMS;
  return m_streams[streamIndex].m_data.get();
}

BindingInfo const & AttributeProvider::GetBindingInfo(uint8_t streamIndex) const
{
  ASSERT_LESS(streamIndex, GetStreamCount(), ());
  CHECK_STREAMS;
  return m_streams[streamIndex].m_binding;
}

void AttributeProvider::Advance(uint32_t vertexCount)
{
  ASSERT_LESS_OR_EQUAL(vertexCount, m_vertexCount, ());
  CHECK_STREAMS;

  if (m_vertexCount != vertexCount)
  {
    for (size_t i = 0; i < GetStreamCount(); ++i)
    {
      BindingInfo const & info = m_streams[i].m_binding;
      uint32_t offset = vertexCount * info.GetElementSize();
      void * rawPointer = m_streams[i].m_data.get();
      m_streams[i].m_data = make_ref((void *)(((uint8_t *)rawPointer) + offset));
    }
  }

  m_vertexCount -= vertexCount;
}

void AttributeProvider::InitStream(uint8_t streamIndex,
                                   BindingInfo const & bindingInfo,
                                   ref_ptr<void> data)
{
  ASSERT_LESS(streamIndex, GetStreamCount(), ());
  AttributeStream s;
  s.m_binding = bindingInfo;
  s.m_data = data;
  m_streams[streamIndex] = s;
  INIT_STREAM(streamIndex);
}

void AttributeProvider::Reset(uint32_t vertexCount)
{
  m_vertexCount = vertexCount;
}

void AttributeProvider::UpdateStream(uint8_t streamIndex, ref_ptr<void> data)
{
  ASSERT_LESS(streamIndex, GetStreamCount(), ());
  m_streams[streamIndex].m_data = data;
  INIT_STREAM(streamIndex);
}

#ifdef DEBUG
void AttributeProvider::CheckStreams() const
{
  ASSERT(!base::IsExist(m_checkInfo, false), ("Not all streams initialized"));
}

void AttributeProvider::InitCheckStream(uint8_t streamIndex)
{
  m_checkInfo[streamIndex] = true;
}
#endif
}  // namespace dp
