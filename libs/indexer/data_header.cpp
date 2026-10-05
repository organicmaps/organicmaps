#include "indexer/data_header.hpp"
#include "indexer/scales.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/point_coding.hpp"
#include "coding/varint.hpp"

#include "defines.hpp"

#include <algorithm>

namespace feature
{
namespace
{
template <class Sink, class Cont>
void SaveBytes(Sink & sink, Cont const & cont)
{
  static_assert(sizeof(typename Cont::value_type) == 1);

  auto const count = static_cast<uint32_t>(cont.size());
  WriteVarUint(sink, count);
  if (count > 0)
    sink.Write(&cont[0], count);
}

template <class Source, class Cont>
void LoadBytes(Source & src, Cont & cont)
{
  static_assert(sizeof(typename Cont::value_type) == 1);
  ASSERT(cont.empty(), ());

  auto const count = ReadVarUint<uint32_t>(src);
  if (count > 0)
  {
    cont.resize(count);
    src.Read(&cont[0], count);
  }
}
}  // namespace

DataHeader::DataHeader(std::string const & fileName) : DataHeader((FilesContainerR(GetPlatform().GetReader(fileName))))
{}

DataHeader::DataHeader(FilesContainerR const & cont)
{
  Load(cont);
}

serial::GeometryCodingParams DataHeader::GetGeometryCodingParams(int scaleIndex) const
{
  return {static_cast<uint8_t>(m_codingParams.GetCoordBits() - (m_scales.back() - m_scales[scaleIndex]) / 2),
          m_codingParams.GetBasePointUint64()};
}

m2::RectD DataHeader::GetBounds() const
{
  return Int64ToRectObsolete(m_bounds, m_codingParams.GetCoordBits());
}

void DataHeader::SetBounds(m2::RectD const & r)
{
  m_bounds = RectToInt64Obsolete(r, m_codingParams.GetCoordBits());
}

std::pair<int, int> DataHeader::GetScaleRange() const
{
  using namespace scales;

  int const low = 0;
  int const high = GetUpperScale();
  int const worldH = GetUpperWorldScale();
  MapType const type = GetType();

  switch (type)
  {
  case MapType::World: return {low, worldH};
  case MapType::WorldCoasts: return {low, high};
  default:
    ASSERT_EQUAL(type, MapType::Country, ());
    return {worldH + 1, high};

    // Uncomment this to test countries drawing in all scales.
    // return {1, high};
  }
}

void DataHeader::SetFeatureOffsets(FeatureOffsets const & offsets)
{
  ASSERT(std::is_sorted(offsets.begin(), offsets.end()), (offsets));
  m_featureOffsets = offsets;
}

std::pair<uint32_t, uint32_t> DataHeader::GetFeatureRange(FeatureGroup group) const
{
  auto const index = static_cast<size_t>(group);
  ASSERT_LESS(index, m_featureOffsets.size(), ());
  return {index == 0 ? 0 : m_featureOffsets[index - 1], m_featureOffsets[index]};
}

uint32_t DataHeader::GetFeatureCount() const
{
  return m_featureOffsets.back();
}

void DataHeader::Save(FileWriter & w) const
{
  m_codingParams.Save(w);

  WriteVarInt(w, m_bounds.first);
  WriteVarInt(w, m_bounds.second);

  SaveBytes(w, m_scales);
  SaveBytes(w, m_langs);

  WriteVarInt(w, static_cast<int32_t>(m_type));

  for (auto const offset : m_featureOffsets)
    WriteVarUint(w, offset);
}

void DataHeader::Load(FilesContainerR const & cont)
{
  auto const format = version::MwmVersion::Read(cont).GetFormat();
  Load(cont.GetReader(HEADER_FILE_TAG), format >= version::Format::v12);
}

void DataHeader::Load(ModelReaderPtr const & r, bool hasFeatureRanges)
{
  ReaderSource<ModelReaderPtr> src(r);
  m_codingParams.Load(src);

  m_bounds.first = ReadVarInt<int64_t>(src);
  m_bounds.second = ReadVarInt<int64_t>(src);

  LoadBytes(src, m_scales);
  LoadBytes(src, m_langs);

  m_type = static_cast<MapType>(ReadVarInt<int32_t>(src));

  if (m_type < MapType::World || m_type > MapType::Country || m_scales.size() != kMaxScalesCount)
    MYTHROW(CorruptedMwmFile, (r.GetName()));

  if (hasFeatureRanges)
  {
    for (auto & offset : m_featureOffsets)
      offset = ReadVarUint<uint32_t>(src);
    ASSERT(std::is_sorted(m_featureOffsets.begin(), m_featureOffsets.end()), (m_featureOffsets));
  }
  else
  {
    m_featureOffsets = {};
  }
}

std::string DebugPrint(DataHeader::MapType type)
{
  switch (type)
  {
  case DataHeader::MapType::World: return "World";
  case DataHeader::MapType::WorldCoasts: return "WorldCoasts";
  case DataHeader::MapType::Country: return "Country";
  }

  UNREACHABLE();
}
}  // namespace feature
