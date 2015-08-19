#pragma once

#include "coding/mmap_reader.hpp"

#include "std/iostream.hpp"
#include "std/type_traits.hpp"

/// Used to store all world nodes inside temporary index file.
/// To find node by id, just calculate offset inside index file:
/// offset_in_file = sizeof(LatLon) * node_ID
struct LatLon
{
  double lat;
  double lon;
};
static_assert(sizeof(LatLon) == 16, "Invalid structure size");

struct ShortLatLon
{
  int32_t lat;
  int32_t lon;
};
static_assert(sizeof(ShortLatLon) == 8, "Invalid structure size");

struct LatLonPos
{
  uint64_t pos;
  double lat;
  double lon;
};
static_assert(sizeof(LatLonPos) == 24, "Invalid structure size");

struct ShortLatLonPos
{
  uint64_t pos;
  int32_t lat;
  int32_t lon;
};
static_assert(sizeof(ShortLatLonPos) == 16, "Invalid structure size");


class BasePointStorage
{
protected:
  // helper for select right initialization function
  template <int v>
  struct EnableIf
  {
    enum { value = v };
  };

  size_t m_processedPoint = 0;

public:
  enum EStorageMode {MODE_READ = false, MODE_WRITE = true};

  inline size_t GetProcessedPoint() const { return m_processedPoint; }
  inline void IncProcessedPoint() { ++m_processedPoint; }
};

template < BasePointStorage::EStorageMode ModeT >
class RawFilePointStorage : public BasePointStorage
{
#ifdef OMIM_OS_WINDOWS
  typedef FileReader FileReaderT;
#else
  typedef MmapReader FileReaderT;
#endif

  typename conditional<ModeT, FileWriter, FileReaderT>::type m_file;

public:
  RawFilePointStorage(string const & name) : m_file(name) {}

  template <bool T = (ModeT == BasePointStorage::MODE_WRITE)>
  typename enable_if<T, void>::type AddPoint(uint64_t id, double lat, double lng)
  {
    LatLon ll;
    ll.lat = lat;
    ll.lon = lng;
    m_file.Seek(id * sizeof(ll));
    m_file.Write(&ll, sizeof(ll));

    IncProcessedPoint();
  }

  template <bool T = (ModeT == BasePointStorage::MODE_READ)>
  typename enable_if<T, bool>::type GetPoint(uint64_t id, double & lat, double & lng) const
  {
    LatLon ll;
    m_file.Read(id * sizeof(ll), &ll, sizeof(ll));

    // assume that valid coordinate is not (0, 0)
    if (ll.lat != 0.0 || ll.lon != 0.0)
    {
      lat = ll.lat;
      lng = ll.lon;
      return true;
    }
    else
    {
      LOG(LERROR, ("Node with id = ", id, " not found!"));
      return false;
    }
  }

};

template < BasePointStorage::EStorageMode ModeT >
class RawFileShortPointStorage : public BasePointStorage
{
#ifdef OMIM_OS_WINDOWS
  typedef FileReader FileReaderT;
#else
  typedef MmapReader FileReaderT;
#endif

  typename conditional<ModeT, FileWriter, FileReaderT>::type m_file;

  constexpr static double const kValueOrder = 1E+7;

public:
  RawFileShortPointStorage(string const & name) : m_file(name) {}

  template <bool T = (ModeT == BasePointStorage::MODE_WRITE)>
  typename enable_if<T, void>::type AddPoint(uint64_t id, double lat, double lng)
  {
    int64_t const lat64 = lat * kValueOrder;
    int64_t const lng64 = lng * kValueOrder;

    ShortLatLon ll;
    ll.lat = static_cast<int32_t>(lat64);
    ll.lon = static_cast<int32_t>(lng64);
    CHECK_EQUAL(static_cast<int64_t>(ll.lat), lat64, ("Latitude is out of 32bit boundary!"));
    CHECK_EQUAL(static_cast<int64_t>(ll.lon), lng64, ("Longtitude is out of 32bit boundary!"));

    m_file.Seek(id * sizeof(ll));
    m_file.Write(&ll, sizeof(ll));

    IncProcessedPoint();
  }

  template <bool T = (ModeT == BasePointStorage::MODE_READ)>
  typename enable_if<T, bool>::type GetPoint(uint64_t id, double & lat, double & lng) const
  {
    ShortLatLon ll;
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


template < BasePointStorage::EStorageMode ModeT >
class RawMemShortPointStorage : public BasePointStorage
{
  typename conditional<ModeT, FileWriter, FileReader>::type m_file;

  constexpr static double const kValueOrder = 1E+7;

  typedef ShortLatLon LonLatT;

  vector<LonLatT> m_data;

public:
  RawMemShortPointStorage(string const & name)
  : m_file(name)
  , m_data((size_t)0xFFFFFFFF)
  {
    InitStorage(EnableIf<ModeT>());
  }

  ~RawMemShortPointStorage()
  {
    DoneStorage(EnableIf<ModeT>());
  }

  void InitStorage(EnableIf<MODE_WRITE>) {}

  void InitStorage(EnableIf<MODE_READ>)
  {
    m_file.Read(0, m_data.data(), m_data.size() * sizeof(LonLatT));
  }

  void DoneStorage(EnableIf<MODE_WRITE>)
  {
    m_file.Write(m_data.data(), m_data.size() * sizeof(LonLatT));
  }

  void DoneStorage(EnableIf<MODE_READ>) {}

  template <bool T = (ModeT == BasePointStorage::MODE_WRITE)>
  typename enable_if<T, void>::type AddPoint(uint64_t id, double lat, double lng)
  {
    int64_t const lat64 = lat * kValueOrder;
    int64_t const lng64 = lng * kValueOrder;

    ShortLatLon & ll = m_data[id];
    ll.lat = static_cast<int32_t>(lat64);
    ll.lon = static_cast<int32_t>(lng64);
    CHECK_EQUAL(static_cast<int64_t>(ll.lat), lat64, ("Latitude is out of 32bit boundary!"));
    CHECK_EQUAL(static_cast<int64_t>(ll.lon), lng64, ("Longtitude is out of 32bit boundary!"));

    IncProcessedPoint();
  }

  template <bool T = (ModeT == BasePointStorage::MODE_READ)>
  typename enable_if<T, bool>::type GetPoint(uint64_t id, double & lat, double & lng) const
  {
    ShortLatLon const & ll = m_data[id];
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

template < BasePointStorage::EStorageMode ModeT >
class MapFilePointStorage : public BasePointStorage
{
  typename conditional<ModeT, FileWriter, FileReader>::type m_file;
  typedef unordered_map<uint64_t, pair<double, double> > ContainerT;
  ContainerT m_map;

public:
  MapFilePointStorage(string const & name) : m_file(name)
  {
    InitStorage(EnableIf<ModeT>());
  }

  void InitStorage(EnableIf<MODE_WRITE>) {}

  void InitStorage(EnableIf<MODE_READ>)
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
    LatLonPos ll;
    ll.pos = id;
    ll.lat = lat;
    ll.lon = lng;
    m_file.Write(&ll, sizeof(ll));
    
    IncProcessedPoint();
  }

  bool GetPoint(uint64_t id, double & lat, double & lng) const
  {
    auto i = m_map.find(id);
    if (i == m_map.end())
      return false;
    lat = i->second.first;
    lng = i->second.second;
    return true;
  }
};

template < BasePointStorage::EStorageMode ModeT >
class MapFileShortPointStorage : public BasePointStorage
{
  typename conditional<ModeT, FileWriter, FileReader>::type m_file;
  typedef unordered_map<uint64_t, pair<int32_t, int32_t> > ContainerT;
  ContainerT m_map;

  constexpr static double const kValueOrder = 1E+7;

public:
  MapFileShortPointStorage(string const & name) : m_file(name+".short")
  {
    InitStorage(EnableIf<ModeT>());
  }

  void InitStorage(EnableIf<MODE_WRITE>) {}

  void InitStorage(EnableIf<MODE_READ>)
  {
    LOG(LINFO, ("Nodes reading is started"));

    uint64_t const count = m_file.Size();

    uint64_t pos = 0;
    while (pos < count)
    {
      ShortLatLonPos ll;
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

    ShortLatLonPos ll;
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


