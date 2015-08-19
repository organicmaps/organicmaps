#pragma once

#include "coding/mmap_reader.hpp"

#include "std/iostream.hpp"
#include "std/type_traits.hpp"

/// Used to store all world nodes inside temporary index file.
/// To find node by id, just calculate offset inside index file:
/// offset_in_file = sizeof(LatLon) * node_ID
struct LatLon
{
  int32_t lat;
  int32_t lon;
};
static_assert(sizeof(LatLon) == 8, "Invalid structure size");

struct LatLonPos
{
  uint64_t pos;
  int32_t lat;
  int32_t lon;
};
static_assert(sizeof(LatLonPos) == 16, "Invalid structure size");


class BasePointStorage
{
  size_t m_processedPoint = 0;

public:
  enum EStorageMode {MODE_READ = false, MODE_WRITE = true};

  inline size_t GetProcessedPoint() const { return m_processedPoint; }
  inline void IncProcessedPoint() { ++m_processedPoint; }
};

template < BasePointStorage::EStorageMode TMode >
class RawFilePointStorage : public BasePointStorage
{
#ifdef OMIM_OS_WINDOWS
  using TFileReader = FileReader;
#else
  using TFileReader = MmapReader;
#endif

  typename conditional<TMode, FileWriter, TFileReader>::type m_file;

  constexpr static double const kValueOrder = 1E+7;

public:
  RawFilePointStorage(string const & name) : m_file(name) {}

  template <bool T = TMode>
  typename enable_if<T == MODE_WRITE, void>::type AddPoint(uint64_t id, double lat, double lng)
  {
    int64_t const lat64 = lat * kValueOrder;
    int64_t const lng64 = lng * kValueOrder;

    LatLon ll;
    ll.lat = static_cast<int32_t>(lat64);
    ll.lon = static_cast<int32_t>(lng64);
    CHECK_EQUAL(static_cast<int64_t>(ll.lat), lat64, ("Latitude is out of 32bit boundary!"));
    CHECK_EQUAL(static_cast<int64_t>(ll.lon), lng64, ("Longtitude is out of 32bit boundary!"));

    m_file.Seek(id * sizeof(ll));
    m_file.Write(&ll, sizeof(ll));

    IncProcessedPoint();
  }

  template <bool T = TMode>
  typename enable_if<T == MODE_READ, bool>::type GetPoint(uint64_t id, double & lat, double & lng) const
  {
    LatLon ll;
    m_file.Read(id * sizeof(ll), &ll, sizeof(ll));

    // assume that valid coordinate is not (0, 0)
    if (ll.lat != 0.0 || ll.lon != 0.0)
    {
      lat = static_cast<double>(ll.lat) / kValueOrder;
      lng = static_cast<double>(ll.lon) / kValueOrder;
      return true;
    }
    else
    {
      LOG(LERROR, ("Node with id = ", id, " not found!"));
      return false;
    }
  }
};


template < BasePointStorage::EStorageMode TMode >
class RawMemPointStorage : public BasePointStorage
{
  typename conditional<TMode, FileWriter, FileReader>::type m_file;

  constexpr static double const kValueOrder = 1E+7;

  vector<LatLon> m_data;

public:
  RawMemPointStorage(string const & name)
  : m_file(name)
  , m_data((size_t)0xFFFFFFFF)
  {
    InitStorage<TMode>();
  }

  ~RawMemPointStorage()
  {
    DoneStorage<TMode>();
  }

  template <bool T>
  typename enable_if<T == MODE_WRITE, void>::type InitStorage() {}

  template <bool T>
  typename enable_if<T == MODE_READ, void>::type InitStorage()
  {
    m_file.Read(0, m_data.data(), m_data.size() * sizeof(LatLon));
  }

  template <bool T>
  typename enable_if<T == MODE_WRITE, void>::type DoneStorage()
  {
    m_file.Write(m_data.data(), m_data.size() * sizeof(LatLon));
  }

  template <bool T>
  typename enable_if<T == MODE_READ, void>::type DoneStorage() {}

  template <bool T = TMode>
  typename enable_if<T == MODE_WRITE, void>::type AddPoint(uint64_t id, double lat, double lng)
  {
    int64_t const lat64 = lat * kValueOrder;
    int64_t const lng64 = lng * kValueOrder;

    LatLon & ll = m_data[id];
    ll.lat = static_cast<int32_t>(lat64);
    ll.lon = static_cast<int32_t>(lng64);
    CHECK_EQUAL(static_cast<int64_t>(ll.lat), lat64, ("Latitude is out of 32bit boundary!"));
    CHECK_EQUAL(static_cast<int64_t>(ll.lon), lng64, ("Longtitude is out of 32bit boundary!"));

    IncProcessedPoint();
  }

  template <bool T = TMode>
  typename enable_if<T == MODE_READ, bool>::type GetPoint(uint64_t id, double & lat, double & lng) const
  {
    LatLon const & ll = m_data[id];
    // assume that valid coordinate is not (0, 0)
    if (ll.lat != 0.0 || ll.lon != 0.0)
    {
      lat = static_cast<double>(ll.lat) / kValueOrder;
      lng = static_cast<double>(ll.lon) / kValueOrder;
      return true;
    }
    else
    {
      LOG(LERROR, ("Node with id = ", id, " not found!"));
      return false;
    }
  }
};

template < BasePointStorage::EStorageMode TMode >
class MapFilePointStorage : public BasePointStorage
{
  typename conditional<TMode, FileWriter, FileReader>::type m_file;
  unordered_map<uint64_t, pair<int32_t, int32_t>> m_map;

  constexpr static double const kValueOrder = 1E+7;

public:
  MapFilePointStorage(string const & name) : m_file(name+".short")
  {
    InitStorage<TMode>();
  }

  template <bool T>
  typename enable_if<T == MODE_WRITE, void>::type InitStorage() {}

  template <bool T>
  typename enable_if<T == MODE_READ, void>::type InitStorage()
  {
    LOG(LINFO, ("Nodes reading is started"));

    uint64_t const count = m_file.Size();

    uint64_t pos = 0;
    while (pos < count)
    {
      LatLonPos ll;
      m_file.Read(pos, &ll, sizeof(ll));

      m_map.emplace(make_pair(ll.pos, make_pair(ll.lat, ll.lon)));

      pos += sizeof(ll);
    }

    LOG(LINFO, ("Nodes reading is finished"));
  }

  void AddPoint(uint64_t id, double lat, double lng)
  {
    int64_t const lat64 = lat * kValueOrder;
    int64_t const lng64 = lng * kValueOrder;

    LatLonPos ll;
    ll.pos = id;
    ll.lat = static_cast<int32_t>(lat64);
    ll.lon = static_cast<int32_t>(lng64);
    CHECK_EQUAL(static_cast<int64_t>(ll.lat), lat64, ("Latitude is out of 32bit boundary!"));
    CHECK_EQUAL(static_cast<int64_t>(ll.lon), lng64, ("Longtitude is out of 32bit boundary!"));
    m_file.Write(&ll, sizeof(ll));

    IncProcessedPoint();
  }

  bool GetPoint(uint64_t id, double & lat, double & lng) const
  {
    auto i = m_map.find(id);
    if (i == m_map.end())
      return false;
    lat = static_cast<double>(i->second.first) / kValueOrder;
    lng = static_cast<double>(i->second.second) / kValueOrder;
    return true;
  }
};
