#pragma once

#include "coding/file_reader.hpp"
#include "coding/read_write_utils.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace gen
{
template <class T> class Accumulator
{
protected:
  std::vector<T> m_data;

public:
  typedef T ValueT;

  void Add(T const & t) { m_data.push_back(t); }

  template <class TSink> void Flush(TSink & sink) const
  {
    rw::WriteVectorOfPOD(sink, m_data);
  }

  template <class TSource> void Read(TSource & src)
  {
    rw::ReadVectorOfPOD(src, m_data);
  }
};

class OsmID2FeatureID : public Accumulator<std::pair<base::GeoObjectId, uint32_t /* feature id */>>
{
  typedef Accumulator<ValueT> BaseT;

  struct LessID
  {
    bool operator() (ValueT const & r1, ValueT const & r2) const { return r1.first < r2.first; }
    bool operator()(base::GeoObjectId const & r1, ValueT const & r2) const { return r1 < r2.first; }
    bool operator()(ValueT const & r1, base::GeoObjectId const & r2) const { return r1.first < r2; }
  };

public:
  template <class TSink> void Flush(TSink & sink)
  {
    std::sort(m_data.begin(), m_data.end());
    BaseT::Flush(sink);
  }

  /// Find a feature id for an OSM id.
  bool GetFeatureID(base::GeoObjectId const & id, uint32_t & result) const
  {
    auto const it = std::lower_bound(m_data.begin(), m_data.end(), id, LessID());
    if (it == m_data.end() || it->first != id)
        return false;

    result = it->second;
    return true;
  }

  template <class Fn>
  void ForEach(Fn && fn) const
  {
    for (auto const & v : m_data)
      fn(v);
  }

  bool ReadFromFile(std::string const & filename)
  {
    try
    {
      FileReader reader(filename);
      NonOwningReaderSource src(reader);
      Read(src);
    }
    catch (FileReader::Exception const & e)
    {
      LOG(LERROR, ("Exception while reading osm id to feature id mapping from file", filename,
                   ". Msg:", e.Msg()));
      return false;
    }

    return true;
  }
};
}  // namespace gen
