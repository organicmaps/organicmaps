#include "generator/intermediate_data.hpp"

#include <new>
#include <string>

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/utils.hpp"

#include "defines.hpp"

using namespace std;

namespace
{
size_t const kFlushCount = 1024;
double const kValueOrder = 1e7;
string const kShortExtension = ".short";

// An estimation.
// OSM had around 4.1 billion nodes on 2017-11-08,
// see https://wiki.openstreetmap.org/wiki/Stats
size_t const kMaxNodesInOSM = size_t{1} << 33;

void ToLatLon(double lat, double lon, generator::cache::LatLon & ll)
{
  int64_t const lat64 = lat * kValueOrder;
  int64_t const lon64 = lon * kValueOrder;

  CHECK(TestOverflow<int32_t>(lat64), ("Latitude is out of 32bit boundary:", lat64));
  CHECK(TestOverflow<int32_t>(lon64), ("Longitude is out of 32bit boundary:", lon64));
  ll.m_lat = static_cast<int32_t>(lat64);
  ll.m_lon = static_cast<int32_t>(lon64);
}

bool FromLatLon(generator::cache::LatLon const & ll, double & lat, double & lon)
{
  // Assume that a valid coordinate is not (0, 0).
  if (ll.m_lat != 0.0 || ll.m_lon != 0.0)
  {
    lat = static_cast<double>(ll.m_lat) / kValueOrder;
    lon = static_cast<double>(ll.m_lon) / kValueOrder;
    return true;
  }
  lat = 0.0;
  lon = 0.0;
  return false;
}
}  // namespace

namespace generator
{
namespace cache
{
// IndexFileReader ---------------------------------------------------------------------------------
IndexFileReader::IndexFileReader(string const & name) : m_fileReader(name.c_str()) {}

void IndexFileReader::ReadAll()
{
  m_elements.clear();
  size_t fileSize = m_fileReader.Size();
  if (fileSize == 0)
    return;

  LOG_SHORT(LINFO, ("Offsets reading is started for file", m_fileReader.GetName()));
  CHECK_EQUAL(0, fileSize % sizeof(Element), ("Damaged file."));

  try
  {
    m_elements.resize(base::checked_cast<size_t>(fileSize / sizeof(Element)));
  }
  catch (std::bad_alloc const &)
  {
    LOG(LCRITICAL, ("Insufficient memory for required offset map"));
  }

  m_fileReader.Read(0, &m_elements[0], base::checked_cast<size_t>(fileSize));

  sort(m_elements.begin(), m_elements.end(), ElementComparator());

  LOG_SHORT(LINFO, ("Offsets reading is finished"));
}

bool IndexFileReader::GetValueByKey(Key key, Value & value) const
{
  auto it = lower_bound(m_elements.begin(), m_elements.end(), key, ElementComparator());
  if (it != m_elements.end() && it->first == key)
  {
    value = it->second;
    return true;
  }
  return false;
}

// IndexFileWriter ---------------------------------------------------------------------------------
IndexFileWriter::IndexFileWriter(string const & name) : m_fileWriter(name.c_str()) {}

void IndexFileWriter::WriteAll()
{
  if (m_elements.empty())
    return;

  m_fileWriter.Write(&m_elements[0], m_elements.size() * sizeof(Element));
  m_elements.clear();
}

void IndexFileWriter::Add(Key k, Value const & v)
{
  if (m_elements.size() > kFlushCount)
    WriteAll();

  m_elements.emplace_back(k, v);
}

// OSMElementCacheReader ---------------------------------------------------------------------------
OSMElementCacheReader::OSMElementCacheReader(string const & name, bool preload)
  : m_fileReader(name), m_offsets(name + OFFSET_EXT), m_name(name), m_preload(preload)
{
  if (!m_preload)
    return;
  size_t sz = m_fileReader.Size();
  m_data.resize(sz);
  m_fileReader.Read(0, m_data.data(), sz);
}

void OSMElementCacheReader::LoadOffsets() { m_offsets.ReadAll(); }

// OSMElementCacheWriter ---------------------------------------------------------------------------
OSMElementCacheWriter::OSMElementCacheWriter(string const & name, bool preload)
  : m_fileWriter(name), m_offsets(name + OFFSET_EXT), m_name(name), m_preload(preload)
{
}

void OSMElementCacheWriter::SaveOffsets() { m_offsets.WriteAll(); }

// RawFilePointStorageMmapReader -------------------------------------------------------------------
RawFilePointStorageMmapReader::RawFilePointStorageMmapReader(string const & name)
  : m_mmapReader(name)
{
}

bool RawFilePointStorageMmapReader::GetPoint(uint64_t id, double & lat, double & lon) const
{
  LatLon ll;
  m_mmapReader.Read(id * sizeof(ll), &ll, sizeof(ll));

  bool ret = FromLatLon(ll, lat, lon);
  if (!ret)
    LOG(LERROR, ("Node with id =", id, "not found!"));
  return ret;
}

// RawFilePointStorageWriter -----------------------------------------------------------------------
RawFilePointStorageWriter::RawFilePointStorageWriter(string const & name) : m_fileWriter(name)
{
}

void RawFilePointStorageWriter::AddPoint(uint64_t id, double lat, double lon)
{
  LatLon ll;
  ToLatLon(lat, lon, ll);

  m_fileWriter.Seek(id * sizeof(ll));
  m_fileWriter.Write(&ll, sizeof(ll));

  ++m_numProcessedPoints;
}

// RawMemPointStorageReader ------------------------------------------------------------------------
RawMemPointStorageReader::RawMemPointStorageReader(string const & name)
  : m_fileReader(name), m_data(kMaxNodesInOSM)
{
  static_assert(sizeof(size_t) == 8, "This code is only for 64-bit architectures");
  m_fileReader.Read(0, m_data.data(), m_data.size() * sizeof(LatLon));
}

bool RawMemPointStorageReader::GetPoint(uint64_t id, double & lat, double & lon) const
{
  LatLon const & ll = m_data[id];
  bool ret = FromLatLon(ll, lat, lon);
  if (!ret)
    LOG(LERROR, ("Node with id =", id, "not found!"));
  return ret;
}

// RawMemPointStorageWriter ------------------------------------------------------------------------
RawMemPointStorageWriter::RawMemPointStorageWriter(string const & name)
  : m_fileWriter(name), m_data(kMaxNodesInOSM)
{
}

RawMemPointStorageWriter::~RawMemPointStorageWriter()
{
  m_fileWriter.Write(m_data.data(), m_data.size() * sizeof(LatLon));
}

void RawMemPointStorageWriter::AddPoint(uint64_t id, double lat, double lon)
{
  CHECK_LESS(id, m_data.size(),
             ("Found node with id", id, "which is bigger than the allocated cache size"));

  LatLon & ll = m_data[id];
  ToLatLon(lat, lon, ll);

  ++m_numProcessedPoints;
}

// MapFilePointStorageReader -----------------------------------------------------------------------
MapFilePointStorageReader::MapFilePointStorageReader(string const & name)
  : m_fileReader(name + kShortExtension)
{
  LOG(LINFO, ("Nodes reading is started"));

  uint64_t const count = m_fileReader.Size();

  uint64_t pos = 0;
  LatLonPos llp;
  LatLon ll;
  while (pos < count)
  {
    m_fileReader.Read(pos, &llp, sizeof(llp));
    pos += sizeof(llp);

    ll.m_lat = llp.m_lat;
    ll.m_lon = llp.m_lon;
    m_map.emplace(llp.m_pos, ll);
  }

  LOG(LINFO, ("Nodes reading is finished"));
}

bool MapFilePointStorageReader::GetPoint(uint64_t id, double & lat, double & lon) const
{
  auto const i = m_map.find(id);
  if (i == m_map.cend())
    return false;
  bool ret = FromLatLon(i->second, lat, lon);
  if (!ret)
  {
    LOG(LERROR, ("Inconsistent MapFilePointStorageReader. Node with id =", id,
                 "must exist but was not found"));
  }
  return ret;
}

// MapFilePointStorageWriter -----------------------------------------------------------------------
MapFilePointStorageWriter::MapFilePointStorageWriter(string const & name)
  : m_fileWriter(name + kShortExtension)
{
}

void MapFilePointStorageWriter::AddPoint(uint64_t id, double lat, double lon)
{
  LatLon ll;
  ToLatLon(lat, lon, ll);

  LatLonPos llp;
  llp.m_pos = id;
  llp.m_lat = ll.m_lat;
  llp.m_lon = ll.m_lon;

  m_fileWriter.Write(&llp, sizeof(llp));

  ++m_numProcessedPoints;
}

// IntermediateDataReader
IntermediateDataReader::IntermediateDataReader(shared_ptr<IPointStorageReader> nodes,
                                               feature::GenerateInfo & info) :
  m_nodes(nodes),
  m_ways(info.GetIntermediateFileName(WAYS_FILE), info.m_preloadCache),
  m_relations(info.GetIntermediateFileName(RELATIONS_FILE), info.m_preloadCache),
  m_nodeToRelations(info.GetIntermediateFileName(NODES_FILE, ID2REL_EXT)),
  m_wayToRelations(info.GetIntermediateFileName(WAYS_FILE, ID2REL_EXT))
{
}

void IntermediateDataReader::LoadIndex()
{
  m_ways.LoadOffsets();
  m_relations.LoadOffsets();

  m_nodeToRelations.ReadAll();
  m_wayToRelations.ReadAll();
}


// IntermediateDataWriter
IntermediateDataWriter::IntermediateDataWriter(std::shared_ptr<IPointStorageWriter> nodes,
                                               feature::GenerateInfo & info):
  m_nodes(nodes),
  m_ways(info.GetIntermediateFileName(WAYS_FILE), info.m_preloadCache),
  m_relations(info.GetIntermediateFileName(RELATIONS_FILE), info.m_preloadCache),
  m_nodeToRelations(info.GetIntermediateFileName(NODES_FILE, ID2REL_EXT)),
  m_wayToRelations(info.GetIntermediateFileName(WAYS_FILE, ID2REL_EXT))
{
}

void IntermediateDataWriter::AddRelation(Key id, RelationElement const & e)
{
  string const & relationType = e.GetType();
  if (!(relationType == "multipolygon" || relationType == "route" || relationType == "boundary" ||
        relationType == "associatedStreet" || relationType == "building" ||
        relationType == "restriction"))
  {
    return;
  }

  m_relations.Write(id, e);
  AddToIndex(m_nodeToRelations, id, e.nodes);
  AddToIndex(m_wayToRelations, id, e.ways);
}

void IntermediateDataWriter::SaveIndex()
{
  m_ways.SaveOffsets();
  m_relations.SaveOffsets();

  m_nodeToRelations.WriteAll();
  m_wayToRelations.WriteAll();
}


// Functions
std::shared_ptr<IPointStorageReader>
CreatePointStorageReader(feature::GenerateInfo::NodeStorageType type, string const & name)
{
  switch (type)
  {
  case feature::GenerateInfo::NodeStorageType::File:
    return std::make_shared<RawFilePointStorageMmapReader>(name);
  case feature::GenerateInfo::NodeStorageType::Index:
    return std::make_shared<MapFilePointStorageReader>(name);
  case feature::GenerateInfo::NodeStorageType::Memory:
    return std::make_shared<RawMemPointStorageReader>(name);
  }
  CHECK_SWITCH();
}

std::shared_ptr<IPointStorageWriter>
CreatePointStorageWriter(feature::GenerateInfo::NodeStorageType type, std::string const & name)
{
  switch (type)
  {
  case feature::GenerateInfo::NodeStorageType::File:
    return std::make_shared<RawFilePointStorageWriter>(name);
  case feature::GenerateInfo::NodeStorageType::Index:
    return std::make_shared<MapFilePointStorageWriter>(name);
  case feature::GenerateInfo::NodeStorageType::Memory:
    return std::make_shared<RawMemPointStorageWriter>(name);
  }
  CHECK_SWITCH();
}

}  // namespace cache
}  // namespace generator
