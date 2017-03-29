#pragma once

#include "generator/osm_id.hpp"

#include "coding/read_write_utils.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace gen
{
template <class T> class Accumulator
{
protected:
  vector<T> m_data;

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

class OsmID2FeatureID : public Accumulator<pair<osm::Id, uint32_t /* feature id */>>
{
  typedef Accumulator<ValueT> BaseT;

  struct LessID
  {
    bool operator() (ValueT const & r1, ValueT const & r2) const { return r1.first < r2.first; }
    bool operator() (osm::Id const & r1, ValueT const & r2) const { return r1 < r2.first; }
    bool operator() (ValueT const & r1, osm::Id const & r2) const { return r1.first < r2; }
  };

public:
  template <class TSink> void Flush(TSink & sink)
  {
    sort(m_data.begin(), m_data.end());
    BaseT::Flush(sink);
  }

  /// Find a feature id for an OSM way id. Returns 0 if the feature was not found.
  uint32_t GetRoadFeatureID(uint64_t wayId) const
  {
    osm::Id id = osm::Id::Way(wayId);
    auto const it = lower_bound(m_data.begin(), m_data.end(), id, LessID());
    if (it != m_data.end() && it->first == id)
      return it->second;
    return 0;
  }

  template <class Fn>
  void ForEach(Fn && fn) const
  {
    for (auto const & v : m_data)
      fn(v);
  }
};
}  // namespace gen
