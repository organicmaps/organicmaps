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
// A serializable bidirectional read-only map of FeatureIds from a single
// mwm of a fixed version to GeoObjectIds.
// Currently, only World.mwm of the latest version is supported.
class FeatureIdToGeoObjectIdBimap
{
public:
  friend class FeatureIdToGeoObjectIdSerDes;

  explicit FeatureIdToGeoObjectIdBimap(DataSource const & dataSource);

  bool Load();

  size_t Size() const { return m_map.Size(); }

  bool GetGeoObjectId(FeatureID const & fid, base::GeoObjectId & id);

  bool GetFeatureID(base::GeoObjectId const & id, FeatureID & fid);

  // todo(@m) Rename to ForEachEntryForTesting or, better yet, remove.
  template <typename Fn>
  void ForEachEntry(Fn && fn) const
  {
    m_map.ForEachEntry(std::forward<Fn>(fn));
  }

private:
  DataSource const & m_dataSource;
  MwmSet::MwmId m_mwmId;

  base::BidirectionalMap<uint32_t, base::GeoObjectId> m_map;
};

using FeatureIdToGeoObjectIdBimapBuilder = base::BidirectionalMap<uint32_t, base::GeoObjectId>;

class FeatureIdToGeoObjectIdSerDes
{
public:
  template <typename Sink>
  static void Serialize(Sink & sink, FeatureIdToGeoObjectIdBimapBuilder const & map)
  {
    WriteToSink(sink, base::checked_cast<uint32_t>(map.Size()));
    map.ForEachEntry([&sink](uint32_t const fid, base::GeoObjectId gid) {
      WriteVarUint(sink, fid);
      WriteVarUint(sink, gid.GetEncodedId());
    });
  }

  template <typename Source>
  static void Deserialize(Source & src, FeatureIdToGeoObjectIdBimap & map)
  {
    map.m_map.Clear();
    auto const numEntries = ReadPrimitiveFromSource<uint32_t>(src);
    for (size_t i = 0; i < numEntries; ++i)
    {
      auto const fid = ReadVarUint<uint32_t>(src);
      auto const gid = ReadVarUint<uint64_t>(src);
      map.m_map.Add(fid, base::GeoObjectId(gid));
    }
  }
};
}  // namespace indexer
