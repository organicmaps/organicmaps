#pragma once

#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"

#include "coding/files_container.hpp"
#include "coding/map_uint32_to_val.hpp"
#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/bidirectional_map.hpp"
#include "base/checked_cast.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class DataSource;

namespace indexer
{
// An in-memory implementation of the data structure that provides a
// serializable mapping of FeatureIDs to base::GeoObjectIds and back.
using FeatureIdToGeoObjectIdBimapMem = base::BidirectionalMap<uint32_t, base::GeoObjectId>;

// A unidirectional read-only map of FeatureIds from a single
// mwm of a fixed version to GeoObjectIds.
// Currently, only World.mwm of the latest version is supported.
class FeatureIdToGeoObjectIdOneWay
{
public:
  friend class FeatureIdToGeoObjectIdSerDes;

  explicit FeatureIdToGeoObjectIdOneWay(DataSource const & dataSource);

  bool Load();

  bool GetGeoObjectId(FeatureID const & fid, base::GeoObjectId & id);

  template <typename Fn>
  void ForEachEntry(Fn && fn) const
  {
    if (!m_mwmId.IsAlive())
      return;

    if (m_mapNodes != nullptr)
      m_mapNodes->ForEach([&](uint32_t fid, uint64_t serialId) { fn(fid, base::MakeOsmNode(serialId)); });
    if (m_mapWays != nullptr)
      m_mapWays->ForEach([&](uint32_t fid, uint64_t serialId) { fn(fid, base::MakeOsmWay(serialId)); });
    if (m_mapRelations != nullptr)
      m_mapRelations->ForEach([&](uint32_t fid, uint64_t serialId) { fn(fid, base::MakeOsmRelation(serialId)); });
  }

  using Id2OsmMapT = MapUint32ToValue<uint64_t>;

private:
  DataSource const & m_dataSource;
  MwmSet::MwmId m_mwmId;
  FilesContainerR::TReader m_reader;

  std::unique_ptr<Id2OsmMapT> m_mapNodes;
  std::unique_ptr<Id2OsmMapT> m_mapWays;
  std::unique_ptr<Id2OsmMapT> m_mapRelations;

  std::unique_ptr<Reader> m_nodesReader;
  std::unique_ptr<Reader> m_waysReader;
  std::unique_ptr<Reader> m_relationsReader;
};

// A bidirectional read-only map of FeatureIds from a single
// mwm of a fixed version to GeoObjectIds.
// Currently, only World.mwm of the latest version is supported.
// This class will likely be much heavier on RAM than FeatureIdToGeoObjectIdOneWay.
class FeatureIdToGeoObjectIdTwoWay
{
public:
  friend class FeatureIdToGeoObjectIdSerDes;

  explicit FeatureIdToGeoObjectIdTwoWay(DataSource const & dataSource);

  bool Load();

  bool GetGeoObjectId(FeatureID const & fid, base::GeoObjectId & id);

  bool GetFeatureID(base::GeoObjectId const & id, FeatureID & fid);

  template <typename Fn>
  void ForEachEntry(Fn && fn) const
  {
    if (!m_mwmId.IsAlive())
      return;

    m_map.ForEachEntry(std::forward<Fn>(fn));
  }

private:
  DataSource const & m_dataSource;
  MwmSet::MwmId m_mwmId;

  FeatureIdToGeoObjectIdBimapMem m_map;
};

// Section format.
//
//   Versions supported   Name                            Offset in bytes      Size in bytes
//   all                  magic (to ease the debugging)   0                    8
//   all                  version                         8                    1
//   v0                   header_v0                       16                   20
//   v0                   fid -> osm nodes mapping        saved in header_v0
//   v0                   fid -> osm ways mapping         saved in header_v0
//   v0                   fid -> osm relations mapping    saved in header_v0
//
// All offsets are aligned to 8 bytes.
// All integer values larger than a byte are little-endian.
class FeatureIdToGeoObjectIdSerDes
{
public:
  enum class Version : uint8_t
  {
    V0,
  };

  inline static std::string const kHeaderMagic = "mwmftosm";
  inline static Version const kLatestVersion = FeatureIdToGeoObjectIdSerDes::Version::V0;
  inline static size_t const kMagicAndVersionSize = 9;
  inline static size_t const kHeaderOffset = 16;

  struct HeaderV0
  {
    template <typename Sink>
    void Write(Sink & sink)
    {
      WriteToSink(sink, m_numEntries);

      WriteToSink(sink, m_nodesOffset);
      WriteToSink(sink, m_waysOffset);
      WriteToSink(sink, m_relationsOffset);
      WriteToSink(sink, m_typesSentinelOffset);
    }

    template <typename Source>
    void Read(Source & src)
    {
      m_numEntries = ReadPrimitiveFromSource<uint32_t>(src);

      m_nodesOffset = ReadPrimitiveFromSource<uint32_t>(src);
      m_waysOffset = ReadPrimitiveFromSource<uint32_t>(src);
      m_relationsOffset = ReadPrimitiveFromSource<uint32_t>(src);
      m_typesSentinelOffset = ReadPrimitiveFromSource<uint32_t>(src);
    }

    uint32_t m_numEntries = 0;

    // All offsets are relative to the start of the payload (which may differ
    // from the end of the header because of padding).
    uint32_t m_nodesOffset = 0;
    uint32_t m_waysOffset = 0;
    uint32_t m_relationsOffset = 0;
    uint32_t m_typesSentinelOffset = 0;
  };

  static_assert(sizeof(HeaderV0) == 20, "");

  template <typename Sink>
  static void Serialize(Sink & sink, FeatureIdToGeoObjectIdBimapMem const & map)
  {
    auto const startPos = sink.Pos();
    sink.Write(kHeaderMagic.data(), kHeaderMagic.size());
    WriteToSink(sink, static_cast<uint8_t>(kLatestVersion));
    CHECK_EQUAL(sink.Pos() - startPos, kMagicAndVersionSize, ());
    WriteZeroesToSink(sink, kHeaderOffset - kMagicAndVersionSize);
    CHECK_EQUAL(sink.Pos() - startPos, kHeaderOffset, ());

    switch (kLatestVersion)
    {
    case Version::V0:
    {
      HeaderV0 header;
      header.Write(sink);
      EnsurePadding(sink, startPos);

      SerializeV0(sink, map, header);

      auto savedPos = sink.Pos();
      sink.Seek(startPos + kHeaderOffset);
      header.Write(sink);
      sink.Seek(savedPos);
    }
      return;
    default: UNREACHABLE();
    }
  }

  template <typename Reader, typename Map>
  static bool Deserialize(Reader & reader, Map & map)
  {
    ReaderSource<decltype(reader)> src(reader);

    if (src.Size() < kHeaderOffset)
    {
      LOG(LINFO, ("Unable to deserialize FeatureToOsm map: wrong header magic or version"));
      return false;
    }
    std::string magic(kHeaderMagic.size(), '\0');
    src.Read(&magic[0], magic.size());
    if (magic != kHeaderMagic)
    {
      LOG(LINFO, ("Unable to deserialize FeatureToOsm map: wrong header magic:", magic));
      return false;
    }

    auto const version = static_cast<Version>(ReadPrimitiveFromSource<uint8_t>(src));
    src.Skip(kHeaderOffset - kMagicAndVersionSize);

    switch (version)
    {
    case Version::V0:
    {
      HeaderV0 header;
      header.Read(src);
      coding::SkipPadding(src);
      DeserializeV0(*src.CreateSubReader(), header, map);
    }
      return true;
    default: LOG(LINFO, ("Unable to deserialize FeatureToOsm map: unknown version")); return false;
    }
  }

  template <typename Sink>
  static void SerializeV0(Sink & sink, FeatureIdToGeoObjectIdBimapMem const & map, HeaderV0 & header)
  {
    using Type = base::GeoObjectId::Type;
    auto const startPos = base::checked_cast<uint32_t>(sink.Pos());
    SerializeV0(sink, Type::OsmNode, map, header.m_nodesOffset);
    SerializeV0(sink, Type::OsmWay, map, header.m_waysOffset);
    SerializeV0(sink, Type::OsmRelation, map, header.m_relationsOffset);

    header.m_numEntries = static_cast<uint32_t>(map.Size());

    header.m_nodesOffset -= startPos;
    header.m_waysOffset -= startPos;
    header.m_relationsOffset -= startPos;
    header.m_typesSentinelOffset = base::checked_cast<uint32_t>(sink.Pos()) - startPos;
  }

  template <typename Sink>
  static void SerializeV0(Sink & sink, base::GeoObjectId::Type type, FeatureIdToGeoObjectIdBimapMem const & map,
                          uint32_t & offset)
  {
    offset = base::checked_cast<uint32_t>(sink.Pos());
    std::vector<std::pair<uint32_t, base::GeoObjectId>> entries;
    entries.reserve(map.Size());
    type = NormalizedType(type);
    map.ForEachEntry([&entries, type](uint32_t const fid, base::GeoObjectId gid)
    {
      if (NormalizedType(gid.GetType()) == type)
        entries.emplace_back(fid, gid);
    });

    std::sort(entries.begin(), entries.end());

    MapUint32ToValueBuilder<uint64_t> builder;

    for (auto const & entry : entries)
      builder.Put(entry.first, entry.second.GetSerialId());

    builder.Freeze(sink, WriteBlockCallback);

    EnsurePadding(sink, offset);
  }

  using Id2OsmMapT = FeatureIdToGeoObjectIdOneWay::Id2OsmMapT;

  template <typename Reader>
  static void DeserializeV0(Reader & reader, HeaderV0 const & header, FeatureIdToGeoObjectIdOneWay & map)
  {
    auto const nodesSize = header.m_waysOffset - header.m_nodesOffset;
    auto const waysSize = header.m_relationsOffset - header.m_waysOffset;
    auto const relationsSize = header.m_typesSentinelOffset - header.m_relationsOffset;

    map.m_nodesReader = reader.CreateSubReader(header.m_nodesOffset, nodesSize);
    map.m_waysReader = reader.CreateSubReader(header.m_waysOffset, waysSize);
    map.m_relationsReader = reader.CreateSubReader(header.m_relationsOffset, relationsSize);

    map.m_mapNodes = Id2OsmMapT::Load(*map.m_nodesReader, ReadBlockCallback);
    map.m_mapWays = Id2OsmMapT::Load(*map.m_waysReader, ReadBlockCallback);
    map.m_mapRelations = Id2OsmMapT::Load(*map.m_relationsReader, ReadBlockCallback);
  }

  template <typename Reader>
  static void DeserializeV0(Reader & reader, HeaderV0 const & header, FeatureIdToGeoObjectIdBimapMem & memMap)
  {
    using Type = base::GeoObjectId::Type;

    memMap.Clear();

    auto const nodesSize = header.m_waysOffset - header.m_nodesOffset;
    auto const waysSize = header.m_relationsOffset - header.m_waysOffset;
    auto const relationsSize = header.m_typesSentinelOffset - header.m_relationsOffset;

    DeserializeV0ToMem(*reader.CreateSubReader(header.m_nodesOffset, nodesSize), Type::ObsoleteOsmNode, memMap);
    DeserializeV0ToMem(*reader.CreateSubReader(header.m_waysOffset, waysSize), Type::ObsoleteOsmWay, memMap);
    DeserializeV0ToMem(*reader.CreateSubReader(header.m_relationsOffset, relationsSize), Type::ObsoleteOsmRelation,
                       memMap);
  }

  template <typename Reader>
  static void DeserializeV0ToMem(Reader & reader, base::GeoObjectId::Type type, FeatureIdToGeoObjectIdBimapMem & memMap)
  {
    auto const map = MapUint32ToValue<uint64_t>::Load(reader, ReadBlockCallback);
    CHECK(map, ());
    map->ForEach([&](uint32_t fid, uint64_t & serialId) { memMap.Add(fid, base::GeoObjectId(type, serialId)); });
  }

private:
  template <typename Sink>
  static void EnsurePadding(Sink & sink, uint64_t startPos)
  {
    uint64_t bytesWritten = sink.Pos() - startPos;
    coding::WritePadding(sink, bytesWritten);
  }

  static base::GeoObjectId::Type NormalizedType(base::GeoObjectId::Type type)
  {
    using Type = base::GeoObjectId::Type;
    switch (type)
    {
    case Type::ObsoleteOsmNode: return Type::OsmNode;
    case Type::ObsoleteOsmWay: return Type::OsmWay;
    case Type::ObsoleteOsmRelation: return Type::OsmRelation;
    default: return type;
    }
  }

  static void ReadBlockCallback(NonOwningReaderSource & src, uint32_t blockSize, std::vector<uint64_t> & values)
  {
    values.reserve(blockSize);
    while (src.Size() > 0)
      values.push_back(ReadVarUint<uint64_t>(src));
  }

  static void WriteBlockCallback(Writer & writer, std::vector<uint64_t>::const_iterator begin,
                                 std::vector<uint64_t>::const_iterator end)
  {
    for (auto it = begin; it != end; ++it)
      WriteVarUint(writer, *it);
  }
};
}  // namespace indexer
