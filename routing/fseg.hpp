#pragma once

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "std/cstdint.hpp"
#include "std/limits.hpp"

namespace routing
{
class FSegId final
{
  static uint32_t constexpr kInvalidFeatureId = numeric_limits<uint32_t>::max();

public:
  static FSegId MakeInvalid() { return FSegId(kInvalidFeatureId, 0); }

  FSegId() : m_featureId(kInvalidFeatureId), m_segId(0) {}

  FSegId(uint32_t featureId, uint32_t segId) : m_featureId(featureId), m_segId(segId) {}

  uint32_t GetFeatureId() const { return m_featureId; }

  uint32_t GetSegId() const { return m_segId; }

  bool IsValid() const { return m_featureId != kInvalidFeatureId; }

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, m_featureId);
    WriteToSink(sink, m_segId);
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    m_featureId = ReadPrimitiveFromSource<decltype(m_featureId)>(src);
    m_segId = ReadPrimitiveFromSource<decltype(m_segId)>(src);
  }

private:
  uint32_t m_featureId;
  uint32_t m_segId;
};
}  // namespace routing
