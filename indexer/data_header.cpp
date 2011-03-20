#include "data_header.hpp"
#include "point_to_int64.hpp"
#include "scales.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"
#include "../coding/write_to_sink.hpp"
#include "../coding/varint.hpp"

#include "../base/start_mem_debug.hpp"


namespace feature
{
  DataHeader::DataHeader()
  {
    Reset();
  }

  void DataHeader::Reset()
  {
  }

  void DataHeader::SetBase(m2::PointD const & p)
  {
    m_base = PointToInt64(p.x, p.y);
  }

  m2::RectD const DataHeader::GetBounds() const
  {
    return Int64ToRect(m_bounds);
  }

  void DataHeader::SetBounds(m2::RectD const & r)
  {
    m_bounds = RectToInt64(r);
  }

  void DataHeader::SetScales(int * arr)
  {
    for (int i = 0; i < m_scales.size(); ++i)
      m_scales[i] = static_cast<uint8_t>(arr[i]);
  }

  pair<int, int> DataHeader::GetScaleRange() const
  {
    pair<int, int> ret(0, scales::GetUpperScale());

    int const bound = scales::GetUpperWorldScale();

    if (m_scales.front() > bound)
      ret.first = bound+1;
    if (m_scales.back() <= bound)
      ret.second = bound;

    return ret;
  }

  void DataHeader::Save(FileWriter & w) const
  {
    WriteToSink(w, m_base);
    WriteVarInt(w, m_bounds.first - m_base);
    WriteVarInt(w, m_bounds.second - m_base);
    w.Write(m_scales.data(), m_scales.size());
  }

  void DataHeader::Load(FileReader const & r)
  {
    ReaderSource<FileReader> src(r);
    m_base = ReadPrimitiveFromSource<int64_t>(src);
    m_bounds.first = ReadVarInt<int64_t>(src) + m_base;
    m_bounds.second = ReadVarInt<int64_t>(src) + m_base;
    src.Read(m_scales.data(), m_scales.size());
  }
}
