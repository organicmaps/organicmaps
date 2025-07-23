#pragma once
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include "geometry/mercator.hpp"

#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace generator
{

class WaysIDHolder
{
  std::vector<uint64_t> m_ways;

public:
  void AddWay(OsmElement const & elem)
  {
    CHECK_LESS(elem.m_id, 1ULL << 63, ());
    // store reversed flag in id
    m_ways.push_back((elem.m_id << 1) | (elem.GetTag("oneway") == "-1" ? 1 : 0));
  }

  using IDRInterfacePtr = std::shared_ptr<cache::IntermediateDataReaderInterface>;
  template <class FnT>
  void ForEachWayWithIndex(FnT && fn, IDRInterfacePtr const & cache) const
  {
    for (size_t i = 0; i < m_ways.size(); ++i)
    {
      uint64_t const id = m_ways[i] >> 1;
      WayElement way(id);
      CHECK(cache->GetWay(id, way), ());

      if ((m_ways[i] & 1) != 0)
        std::reverse(way.m_nodes.begin(), way.m_nodes.end());

      fn(id, way.m_nodes, i);
    }
  }

  template <class FnT>
  void ForEachWay(FnT && fn, IDRInterfacePtr const & cache) const
  {
    ForEachWayWithIndex([&fn](uint64_t id, std::vector<uint64_t> const & nodes, size_t) { fn(id, nodes); }, cache);
  }

  void MergeInto(WaysIDHolder & holder) const
  {
    holder.m_ways.insert(holder.m_ways.end(), m_ways.begin(), m_ways.end());
  }
};

template <class T>
class WayNodesMapper
{
public:
  struct Entry
  {
    m2::PointU m_coord;
    T m_t;

    template <class Sink>
    void Write(Sink & sink, uint64_t wayID, uint32_t nodeIdx) const
    {
      WriteToSink(sink, wayID);
      WriteToSink(sink, nodeIdx);
      WriteToSink(sink, m_coord.x);
      WriteToSink(sink, m_coord.y);
      Save(sink, m_t);
    }

    template <class Source>
    void Read(Source & src, uint64_t & wayID, uint32_t & nodeIdx)
    {
      wayID = ReadPrimitiveFromSource<uint64_t>(src);
      nodeIdx = ReadPrimitiveFromSource<uint32_t>(src);
      m_coord.x = ReadPrimitiveFromSource<uint32_t>(src);
      m_coord.y = ReadPrimitiveFromSource<uint32_t>(src);
      Load(src, m_t);
    }
  };

private:
  std::unordered_map<uint64_t, Entry> m_nodes;

  static m2::PointU EncodePoint(ms::LatLon ll)
  {
    // Convert to mercator, because will compare with compressed FeatureType points then.
    return PointDToPointU(mercator::FromLatLon(ll), kPointCoordBits, mercator::Bounds::FullRect());
  }

public:
  void Add(uint64_t id, ms::LatLon ll, T t)
  {
    CHECK(m_nodes.emplace(id, Entry{EncodePoint(ll), std::move(t)}).second, ());
  }

  void MergeInto(WayNodesMapper & mapper) const { mapper.m_nodes.insert(m_nodes.begin(), m_nodes.end()); }

  Entry const * Find(uint64_t id) const
  {
    auto it = m_nodes.find(id);
    if (it != m_nodes.end())
      return &(it->second);
    return nullptr;
  }

  template <class Source, class FnT>
  static void Deserialize(Source & src, FnT && fn)
  {
    size_t count = ReadPrimitiveFromSource<uint64_t>(src);
    while (count-- > 0)
    {
      Entry e;
      uint64_t wayID;
      uint32_t nodeIdx;
      e.Read(src, wayID, nodeIdx);
      fn(wayID, nodeIdx, e);
    }
  }
};

template <class T>
class WaysMapper
{
  std::unordered_map<uint64_t, T> m_ways;

public:
  void Add(uint64_t id, T t) { CHECK(m_ways.emplace(id, std::move(t)).second, ()); }

  T * FindOrInsert(uint64_t id)
  {
    auto it = m_ways.emplace(id, T()).first;
    return &(it->second);
  }

  void MergeInto(WaysMapper & mapper) const { mapper.m_ways.insert(m_ways.begin(), m_ways.end()); }

  template <class Sink>
  void Serialize(Sink & sink) const
  {
    WriteToSink(sink, uint64_t(m_ways.size()));
    for (auto const & e : m_ways)
    {
      WriteToSink(sink, e.first);
      Save(sink, e.second);
    }
  }

  template <class Source, class FnT>
  static void Deserialize(Source & src, FnT && fn)
  {
    size_t count = ReadPrimitiveFromSource<uint64_t>(src);
    while (count-- > 0)
    {
      uint64_t const id = ReadPrimitiveFromSource<uint64_t>(src);
      T e;
      Load(src, e);
      fn(id, std::move(e));
    }
  }
};

template <typename SizeT>
class SizeWriter
{
  SizeT m_sizePos;

public:
  template <class Sink>
  void Reserve(Sink & sink)
  {
    m_sizePos = sink.Pos();
    WriteToSink(sink, SizeT(0));
  }

  template <class Sink>
  void Write(Sink & sink, SizeT sz)
  {
    auto const savedPos = sink.Pos();
    sink.Seek(m_sizePos);
    WriteToSink(sink, sz);
    sink.Seek(savedPos);
  }
};

}  // namespace generator
