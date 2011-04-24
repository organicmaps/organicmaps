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
    m_codingParams.Save(w);
    WriteVarInt(w, m_bounds.first - m_codingParams.GetBasePointInt64());
    WriteVarInt(w, m_bounds.second - m_codingParams.GetBasePointInt64());
    w.Write(m_scales.data(), m_scales.size());
  }

  void DataHeader::Load(FileReader const & r)
  {
    ReaderSource<FileReader> src(r);
    m_codingParams.Load(src);
    m_bounds.first = ReadVarInt<int64_t>(src) + m_codingParams.GetBasePointInt64();
    m_bounds.second = ReadVarInt<int64_t>(src) + m_codingParams.GetBasePointInt64();
    src.Read(m_scales.data(), m_scales.size());
  }
}
