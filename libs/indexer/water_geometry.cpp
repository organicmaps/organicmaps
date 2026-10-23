#include "indexer/water_geometry.hpp"
#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/write_to_sink.hpp"

#include "geometry/region2d.hpp"
#include "geometry/segment2d.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <bit>

namespace water
{
bool IsLine(std::vector<uint32_t> const & types)
{
  static ftypes::BaseCheckerEx const checker({{"waterway", "river"},
                                              {"waterway", "canal"},
                                              {"waterway", "stream"},
                                              {"waterway", "ditch"},
                                              {"waterway", "drain"},
                                              {"waterway", "fish_pass"}});
  return checker(types);
}

bool IsArea(std::vector<uint32_t> const & types)
{
  static ftypes::BaseCheckerEx const checker({{"natural", "water"}, {"natural", "coastline"}});
  return checker(types);
}

bool IsCoastline(std::vector<uint32_t> const & types)
{
  return ftypes::IsCoastlineChecker::Instance()(types);
}

namespace
{
template <typename Sink>
void WritePoint(Sink & sink, m2::PointD const & p)
{
  WriteToSink(sink, std::bit_cast<uint64_t>(p.x));
  WriteToSink(sink, std::bit_cast<uint64_t>(p.y));
}
template <typename Source>
m2::PointD ReadPoint(Source & source)
{
  auto const x = std::bit_cast<double>(ReadPrimitiveFromSource<uint64_t>(source));
  auto const y = std::bit_cast<double>(ReadPrimitiveFromSource<uint64_t>(source));
  return {x, y};
}
template <typename Sink>
void WriteRings(Sink & sink, std::vector<std::vector<m2::PointD>> const & rings)
{
  WriteToSink(sink, static_cast<uint32_t>(rings.size()));
  for (auto const & ring : rings)
  {
    WriteToSink(sink, static_cast<uint32_t>(ring.size()));
    for (auto const & p : ring)
      WritePoint(sink, p);
  }
}
template <typename Source>
bool ReadRings(Source & source, size_t & budget, std::vector<std::vector<m2::PointD>> & rings)
{
  auto const count = ReadPrimitiveFromSource<uint32_t>(source);
  if (count > budget)
    return false;
  rings.resize(count);
  for (auto & ring : rings)
  {
    auto const size = ReadPrimitiveFromSource<uint32_t>(source);
    if (size > budget)
      return false;
    budget -= size;
    ring.reserve(size);
    for (uint32_t i = 0; i < size; ++i)
      ring.push_back(ReadPoint(source));
  }
  return true;
}
}  // namespace

m2::RectD Record::GetBounds() const
{
  m2::RectD bounds;
  for (auto const & ring : m_rings)
    for (auto const & point : ring)
      bounds.Add(point);
  return bounds;
}

void WriteSection(Writer & writer, std::vector<std::vector<m2::PointD>> const & coverage,
                  std::vector<Record> const & records)
{
  auto const sectionStart = writer.Pos();
  WriteToSink(writer, kVersion);
  WriteRings(writer, coverage);
  WriteToSink(writer, static_cast<uint32_t>(records.size()));
  // FilesContainer writers are append-only. Calculate offsets before streaming
  // payloads instead of seeking backwards or buffering a country's geometry.
  uint64_t offset = writer.Pos() - sectionStart + records.size() * 48;
  for (auto const & record : records)
  {
    uint64_t size = 1 + 4 + 4 + record.m_nodes.size() * 8;
    for (auto const & ring : record.m_rings)
      size += 4 + ring.size() * 16;
    auto const bounds = record.GetBounds();
    WritePoint(writer, bounds.LeftBottom());
    WritePoint(writer, bounds.RightTop());
    WriteToSink(writer, offset);
    WriteToSink(writer, size);
    offset += size;
  }
  for (auto const & record : records)
  {
    WriteToSink(writer, static_cast<uint8_t>(record.m_kind));
    WriteRings(writer, record.m_rings);
    WriteToSink(writer, static_cast<uint32_t>(record.m_nodes.size()));
    for (auto const node : record.m_nodes)
      WriteToSink(writer, node);
  }
}

Reader::Reader(ModelReaderPtr reader) : Reader(ReaderPtr<::Reader>(reader.GetPtr()->CreateSubReader(0, reader.Size())))
{}

Reader::Reader(ReaderPtr<::Reader> reader) : m_reader(std::move(reader))
{
  ReaderSource<ReaderPtr<::Reader>> source(m_reader);
  if (ReadPrimitiveFromSource<uint32_t>(source) != kVersion)
    return;
  size_t budget = 1000000;
  if (!ReadRings(source, budget, m_coverage))
    return;
  auto const count = ReadPrimitiveFromSource<uint32_t>(source);
  // Bound index metadata to 3 MiB, plus the spatial tree. Oversized sections
  // remain unsupported rather than consuming an unbounded amount of RAM.
  if (count > 65536)
    return;
  m_entries.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
  {
    auto const bottom = ReadPoint(source);
    auto const top = ReadPoint(source);
    Entry entry;
    entry.m_bounds = m2::RectD(bottom, top);
    entry.m_offset = ReadPrimitiveFromSource<uint64_t>(source);
    entry.m_size = ReadPrimitiveFromSource<uint64_t>(source);
    m_entries.push_back(entry);
    m_index.Add(i, entry.m_bounds);
  }
  m_supported = true;
}

bool Reader::Covers(m2::RectD const & rect) const
{
  if (!m_supported)
    return false;
  // Accept a rectangle strictly inside one original border ring. A border crossing,
  // enclave or unavailable neighbouring map is never certified from bounding boxes.
  for (auto const & ring : m_coverage)
  {
    m2::RegionD region(ring);
    if (!region.Contains(rect.LeftBottom()) || !region.Contains(rect.LeftTop()) ||
        !region.Contains(rect.RightBottom()) || !region.Contains(rect.RightTop()))
      continue;
    bool crossing = false;
    for (size_t i = 0; i < ring.size(); ++i)
    {
      m2::RectD edge(ring[i], ring[(i + 1) % ring.size()]);
      // Reject even a boundary contact; a conservative false negative is safe.
      if (rect.IsIntersect(edge))
        crossing = true;
    }
    if (!crossing)
      return true;
  }
  return false;
}

bool Reader::Read(m2::RectD const & rect, size_t maxPoints, std::function<bool()> const & cancelled,
                  std::vector<Record> & records) const
{
  records.clear();
  if (!Covers(rect))
    return false;
  std::vector<size_t> selected;
  m_index.ForEachInRect(rect, [&](size_t index) { selected.push_back(index); });
  std::sort(selected.begin(), selected.end());
  for (auto const index : selected)
  {
    if (cancelled())
      return false;
    auto const & entry = m_entries[index];
    ReaderSource<ReaderPtr<::Reader>> source(m_reader.SubReader(entry.m_offset, entry.m_size));
    Record record;
    record.m_kind = static_cast<Kind>(ReadPrimitiveFromSource<uint8_t>(source));
    if (record.m_kind == Kind::Unsupported || !ReadRings(source, maxPoints, record.m_rings))
      return false;
    auto const nodes = ReadPrimitiveFromSource<uint32_t>(source);
    if (record.m_kind == Kind::Line && (record.m_rings.size() != 1 || nodes != record.m_rings.front().size()))
      return false;
    record.m_nodes.reserve(nodes);
    for (uint32_t i = 0; i < nodes; ++i)
      record.m_nodes.push_back(ReadPrimitiveFromSource<uint64_t>(source));
    records.push_back(std::move(record));
  }
  return true;
}
}  // namespace water
