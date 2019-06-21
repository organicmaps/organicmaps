#pragma once

#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"

#include "coding/varint.hpp"

#include "base/bidirectional_map.hpp"
#include "base/checked_cast.hpp"
#include "base/geo_object_id.hpp"

#include <string>
#include <utility>

class DataSource;

namespace indexer
{
// A serializable bidirectional map of FeatureIds from a single
// mwm of a fixed version to GeoObjectIds.
// Currently, only World.mwm of the latest version is supported.
class FeatureIdToGeoObjectIdBimap
{
public:
  explicit FeatureIdToGeoObjectIdBimap(DataSource const & dataSource);

  bool Load();

  bool GetGeoObjectId(FeatureID const & fid, base::GeoObjectId & id);

  bool GetFeatureID(base::GeoObjectId const & id, FeatureID & fid);

  // todo(@m) Rename to ForEachEntryForTesting or, better yet, remove.
  template <typename Fn>
  void ForEachEntry(Fn && fn) const
  {
    m_map.ForEachEntry(std::forward<Fn>(fn));
  }

  template <typename Source>
  void Deserialize(Source & src)
  {
    m_map.Clear();
    auto const numEntries = ReadPrimitiveFromSource<uint32_t>(src);
    for (size_t i = 0; i < numEntries; ++i)
    {
      auto const fid = ReadVarUint<uint32_t>(src);
      auto const gid = ReadVarUint<uint64_t>(src);
      m_map.Add(fid, base::GeoObjectId(gid));
    }
  }

private:
  DataSource const & m_dataSource;
  MwmSet::MwmId m_mwmId;

  base::BidirectionalMap<uint32_t, base::GeoObjectId> m_map;
};

class FeatureIdToGeoObjectIdBimapBuilder
{
public:
  void Add(FeatureID const & fid, base::GeoObjectId const & gid) { m_map.Add(fid.m_index, gid); }

  void Add(uint32_t fid, base::GeoObjectId const & gid) { m_map.Add(fid, gid); }

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    WriteToSink(sink, base::checked_cast<uint32_t>(m_map.Size()));
    m_map.ForEachEntry([&sink](uint32_t const fid, base::GeoObjectId gid) {
      WriteVarUint(sink, fid);
      WriteVarUint(sink, gid.GetEncodedId());
    });
  }

  size_t Size() const { return m_map.Size(); }

  template <typename Fn>
  void ForEachEntry(Fn && fn) const
  {
    m_map.ForEachEntry(std::forward<Fn>(fn));
  }

private:
  base::BidirectionalMap<uint32_t, base::GeoObjectId> m_map;
};
}  // namespace indexer
