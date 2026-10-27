#pragma once

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace water
{
inline char const kSectionTag[] = "water_geometry";
inline uint32_t constexpr kVersion = 1;
bool IsLine(std::vector<uint32_t> const & types);
bool IsArea(std::vector<uint32_t> const & types);
bool IsCoastline(std::vector<uint32_t> const & types);

enum class Kind : uint8_t
{
  Line,
  Area,
  Unsupported
};

struct Record
{
  Kind m_kind = Kind::Unsupported;
  std::vector<std::vector<m2::PointD>> m_rings;
  // Original OSM node ids, in the same order as the line coordinates.
  std::vector<uint64_t> m_nodes;
  m2::RectD GetBounds() const;
};

// Optional section. Coverage is the intersection of the source-extract bounds and
// the unsimplified map borders. Absence of a feature inside coverage means land.
void WriteSection(Writer & writer, std::vector<std::vector<m2::PointD>> const & coverage,
                  std::vector<Record> const & records);

class Reader
{
public:
  explicit Reader(ModelReaderPtr reader);
  explicit Reader(ReaderPtr<::Reader> reader);
  bool IsSupported() const { return m_supported; }
  bool Covers(m2::RectD const & rect) const;
  // Bounded payload extraction; the index is cached, not scanned for each request.
  bool Read(m2::RectD const & rect, size_t maxPoints, std::function<bool()> const & cancelled,
            std::vector<Record> & records) const;

private:
  struct Entry
  {
    m2::RectD m_bounds;
    uint64_t m_offset = 0;
    uint64_t m_size = 0;
  };
  ReaderPtr<::Reader> m_reader;
  bool m_supported = false;
  std::vector<std::vector<m2::PointD>> m_coverage;
  std::vector<Entry> m_entries;
  m4::Tree<size_t> m_index;
};
}  // namespace water
