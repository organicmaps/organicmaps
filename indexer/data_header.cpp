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
    for (size_t i = 0; i < m_scales.size(); ++i)
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

    //int64_t const base = m_codingParams.GetBasePointInt64();
    //WriteVarInt(w, m_bounds.first - base);
    //WriteVarInt(w, m_bounds.second - base);
    WriteToSink(w, m_bounds.first);
    WriteToSink(w, m_bounds.second);

    w.Write(m_scales.data(), m_scales.size());
  }

  void DataHeader::Load(ModelReaderPtr const & r)
  {
    ReaderSource<ModelReaderPtr> src(r);
    m_codingParams.Load(src);

    //int64_t const base = m_codingParams.GetBasePointInt64();
    //m_bounds.first = ReadVarInt<int64_t>(src) + base;
    //m_bounds.second = ReadVarInt<int64_t>(src) + base;
    m_bounds.first = ReadPrimitiveFromSource<int64_t>(src);
    m_bounds.second = ReadPrimitiveFromSource<int64_t>(src);

    src.Read(m_scales.data(), m_scales.size());
  }
}
