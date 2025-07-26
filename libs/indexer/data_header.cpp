#include "indexer/data_header.hpp"
#include "indexer/scales.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/point_coding.hpp"
#include "coding/varint.hpp"

#include "defines.hpp"

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

void DataHeader::Save(FileWriter & w) const
{
  m_codingParams.Save(w);

  WriteVarInt(w, m_bounds.first);
  WriteVarInt(w, m_bounds.second);

  SaveBytes(w, m_scales);
  SaveBytes(w, m_langs);

  WriteVarInt(w, static_cast<int32_t>(m_type));
}

void DataHeader::Load(FilesContainerR const & cont)
{
  Load(cont.GetReader(HEADER_FILE_TAG));
}

void DataHeader::Load(ModelReaderPtr const & r)
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
