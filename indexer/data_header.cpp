#include "data_header.hpp"
#include "point_to_int64.hpp"
#include "scales.hpp"

#include "../defines.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"
#include "../coding/file_container.hpp"
#include "../coding/write_to_sink.hpp"
#include "../coding/varint.hpp"

#include "../base/start_mem_debug.hpp"


namespace feature
{
  serial::CodingParams DataHeader::GetCodingParams(int scaleIndex) const
  {
    return serial::CodingParams(m_codingParams.GetCoordBits() - (m_scales[3] - m_scales[scaleIndex]) / 2,
                                m_codingParams.GetBasePointUint64());
  }

  m2::RectD const DataHeader::GetBounds() const
  {
    return Int64ToRect(m_bounds, m_codingParams.GetCoordBits());
  }

  void DataHeader::SetBounds(m2::RectD const & r)
  {
    m_bounds = RectToInt64(r, m_codingParams.GetCoordBits());
  }

  void DataHeader::SetScales(int * arr)
  {
    for (size_t i = 0; i < m_scales.size(); ++i)
      m_scales[i] = static_cast<uint8_t>(arr[i]);
  }

  pair<int, int> DataHeader::GetScaleRange() const
  {
    using namespace scales;

    int const low = 0;
    int const worldH = GetUpperWorldScale();
    MapType const type = GetType();

    switch (type)
    {
    case world: return make_pair(low, worldH);
    case worldcoasts: return make_pair(low, worldH);
    default:
      ASSERT_EQUAL(type, country, ());
      return make_pair(worldH + 1, GetUpperScale());
      //return make_pair(low, GetUpperScale());
    }
  }

  void DataHeader::Save(FileWriter & w) const
  {
    m_codingParams.Save(w);

    WriteVarInt(w, m_bounds.first);
    WriteVarInt(w, m_bounds.second);

    w.Write(m_scales.data(), m_scales.size());

    WriteVarInt(w, static_cast<int32_t>(m_type));
  }

  void DataHeader::Load(ModelReaderPtr const & r)
  {
    ReaderSource<ModelReaderPtr> src(r);
    m_codingParams.Load(src);

    m_bounds.first = ReadVarInt<int64_t>(src);
    m_bounds.second = ReadVarInt<int64_t>(src);

    src.Read(m_scales.data(), m_scales.size());

    m_type = static_cast<MapType>(ReadVarInt<int32_t>(src));

    m_ver = v2;
  }

  void DataHeader::LoadVer1(ModelReaderPtr const & r)
  {
    ReaderSource<ModelReaderPtr> src(r);
    int64_t const base = ReadPrimitiveFromSource<int64_t>(src);
    m_codingParams = serial::CodingParams(POINT_COORD_BITS, base);

    m_bounds.first = ReadVarInt<int64_t>(src) + base;
    m_bounds.second = ReadVarInt<int64_t>(src) + base;

    src.Read(m_scales.data(), m_scales.size());

    m_ver = v1;
  }
}
