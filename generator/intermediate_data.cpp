#include "generator/intermediate_data.hpp"

#include <exception>
#include <string>

#include "base/logging.hpp"

#include "defines.hpp"

using namespace std;

namespace
{
size_t const kFlushCount = 1024;
double const kValueOrder = 1e7;

void ToLatLon(double lat, double lon, generator::cache::LatLon & ll)
{
  int64_t const lat64 = lat * kValueOrder;
  int64_t const lon64 = lon * kValueOrder;

  ll.lat = static_cast<int32_t>(lat64);
  ll.lon = static_cast<int32_t>(lon64);
  CHECK_EQUAL(static_cast<int64_t>(ll.lat), lat64, ("Latitude is out of 32bit boundary!"));
  CHECK_EQUAL(static_cast<int64_t>(ll.lon), lon64, ("Longtitude is out of 32bit boundary!"));
}

bool FromLatLon(generator::cache::LatLon const & ll, double & lat, double & lon)
{
  // Assume that a valid coordinate is not (0, 0).
  if (ll.lat != 0.0 || ll.lon != 0.0)
  {
    lat = static_cast<double>(ll.lat) / kValueOrder;
    lon = static_cast<double>(ll.lon) / kValueOrder;
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
  catch (exception const &)  // bad_alloc
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
    LOG(LERROR, ("Node with id =", id, " not found!"));
  return ret;
}

// RawFilePointStorageWriter -----------------------------------------------------------------------
RawFilePointStorageWriter::RawFilePointStorageWriter(string const & name) : m_fileWriter(name) {}

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
  : m_fileReader(name), m_data(static_cast<size_t>(1) << 33)
{
  m_fileReader.Read(0, m_data.data(), m_data.size() * sizeof(LatLon));
}

bool RawMemPointStorageReader::GetPoint(uint64_t id, double & lat, double & lon) const
{
  LatLon const & ll = m_data[id];
  bool ret = FromLatLon(ll, lat, lon);
  if (!ret)
    LOG(LERROR, ("Node with id =", id, " not found!"));
  return ret;
}

// RawMemPointStorageWriter ------------------------------------------------------------------------
RawMemPointStorageWriter::RawMemPointStorageWriter(string const & name)
  : m_fileWriter(name), m_data(static_cast<size_t>(1) << 33)
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
  : m_fileReader(name + ".short")
{
  LOG(LINFO, ("Nodes reading is started"));

  uint64_t const count = m_fileReader.Size();

  uint64_t pos = 0;
  while (pos < count)
  {
    LatLonPos ll;
    m_fileReader.Read(pos, &ll, sizeof(ll));

    m_map.emplace(make_pair(ll.pos, make_pair(ll.lat, ll.lon)));

    pos += sizeof(ll);
  }

  LOG(LINFO, ("Nodes reading is finished"));
}

bool MapFilePointStorageReader::GetPoint(uint64_t id, double & lat, double & lon) const
{
  auto i = m_map.find(id);
  if (i == m_map.end())
    return false;
  lat = static_cast<double>(i->second.first) / kValueOrder;
  lon = static_cast<double>(i->second.second) / kValueOrder;
  return true;
}

// MapFilePointStorageWriter -----------------------------------------------------------------------
MapFilePointStorageWriter::MapFilePointStorageWriter(string const & name)
  : m_fileWriter(name + ".short")
{
}

void MapFilePointStorageWriter::AddPoint(uint64_t id, double lat, double lon)
{
  LatLon ll;
  ToLatLon(lat, lon, ll);

  LatLonPos llp;
  llp.pos = id;
  llp.lat = ll.lat;
  llp.lon = ll.lon;

  m_fileWriter.Write(&llp, sizeof(llp));

  ++m_numProcessedPoints;
}
}  // namespace cache
}  // namespace generator
