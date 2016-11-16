#pragma once

#include "routing/fseg.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "std/cstdint.hpp"
#include "std/limits.hpp"
#include "std/vector.hpp"

namespace routing
{
using JointId = uint32_t;
JointId constexpr kInvalidJointId = numeric_limits<JointId>::max();

class Joint final
{
public:
  void AddEntry(FSegId entry) { m_entries.emplace_back(entry); }

  size_t GetSize() const { return m_entries.size(); }

  FSegId const & GetEntry(size_t i) const { return m_entries[i]; }

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, static_cast<uint32_t>(m_entries.size()));
    for (auto const & entry : m_entries)
    {
      entry.Serialize(sink);
    }
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    size_t const size = static_cast<size_t>(ReadPrimitiveFromSource<uint32_t>(src));
    m_entries.resize(size);
    for (size_t i = 0; i < size; ++i)
    {
      m_entries[i].Deserialize(src);
    }
  }

private:
  vector<FSegId> m_entries;
};

class JointOffset final
{
public:
  JointOffset() : m_begin(0), m_end(0) {}

  JointOffset(uint32_t begin, uint32_t end) : m_begin(begin), m_end(end) {}

  uint32_t Begin() const { return m_begin; }

  uint32_t End() const { return m_end; }

  uint32_t Size() const { return m_end - m_begin; }

  void Assign(uint32_t offset)
  {
    m_begin = offset;
    m_end = offset;
  }

  void IncSize() { ++m_end; }

private:
  uint32_t m_begin;
  uint32_t m_end;
};
}  // namespace routing
