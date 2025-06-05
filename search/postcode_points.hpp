#pragma once

#include "search/mwm_context.hpp"
#include "search/search_index_values.hpp"

#include "indexer/centers_table.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/trie.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/string_utils.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace search
{
class PostcodePoints
{
public:
  enum class Version : uint8_t
  {
    V0 = 0,
    Latest = V0
  };

  struct Header
  {
    template <typename Sink>
    void Serialize(Sink & sink) const
    {
      CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::V0), ());
      WriteToSink(sink, static_cast<uint8_t>(m_version));
      WriteToSink(sink, m_trieOffset);
      WriteToSink(sink, m_trieSize);
      WriteToSink(sink, m_pointsOffset);
      WriteToSink(sink, m_pointsSize);
    }

    void Read(Reader & reader);

    Version m_version = Version::Latest;
    // All offsets are relative to the start of the section (offset of header is zero).
    uint32_t m_trieOffset = 0;
    uint32_t m_trieSize = 0;
    uint32_t m_pointsOffset = 0;
    uint32_t m_pointsSize = 0;
  };

  PostcodePoints(MwmValue const & value);

  void Get(strings::UniString const & postcode, std::vector<m2::PointD> & points) const;
  double GetRadius() const { return m_radius; }

private:
  void Get(strings::UniString const & postcode, bool recursive, std::vector<m2::PointD> & points) const;

  Header m_header;
  std::unique_ptr<CentersTable> m_points;
  std::unique_ptr<trie::Iterator<SingleUint64Value>> m_root;
  std::unique_ptr<Reader> m_trieSubReader;
  std::unique_ptr<Reader> m_pointsSubReader;
  double m_radius = 0.0;
};

class PostcodePointsCache
{
public:
  PostcodePoints & Get(MwmContext const & context);
  void Clear() { m_entries.clear(); }

private:
  std::map<MwmSet::MwmId, std::unique_ptr<PostcodePoints>> m_entries;
};
}  // namespace search
