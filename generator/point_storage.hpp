#pragma once

#include "../coding/mmap_reader.hpp"
#include <sqlite3.h>


/// Used to store all world nodes inside temporary index file.
/// To find node by id, just calculate offset inside index file:
/// offset_in_file = sizeof(LatLon) * node_ID
struct LatLon
{
  double lat;
  double lon;
};
STATIC_ASSERT(sizeof(LatLon) == 16);

struct ShortLatLon
{
  int32_t lat;
  int32_t lon;
};
STATIC_ASSERT(sizeof(ShortLatLon) == 8);

struct LatLonPos
{
  uint64_t pos;
  double lat;
  double lon;
};
STATIC_ASSERT(sizeof(LatLonPos) == 24);

struct ShortLatLonPos
{
  uint64_t pos;
  int32_t lat;
  int32_t lon;
};
STATIC_ASSERT(sizeof(ShortLatLonPos) == 16);


class BasePointStorage
{
protected:
  // helper for select right initialization function
  template <int v>
  struct EnableIf
  {
    enum { value = v };
  };

  progress_policy m_progress;

public:
  enum EStorageMode {MODE_READ = false, MODE_WRITE = true};

  BasePointStorage(string const & name, size_t factor)
  {
    m_progress.Begin(name, factor);
  }

  uint64_t GetCount() const { return m_progress.GetCount(); }
};

template < BasePointStorage::EStorageMode ModeT >
class SQLitePointStorage : public BasePointStorage
{
  sqlite3 *m_db;
  sqlite3_stmt *m_prepared_statement;

public:
  SQLitePointStorage(string const & name) : BasePointStorage(name, 1000)
  {
    if( sqlite3_open((name+".sqlite").c_str(), &m_db) != SQLITE_OK ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(m_db));
      sqlite3_close(m_db);
      exit(1);
    }
    InitStorage(EnableIf<ModeT>());
  }

  ~SQLitePointStorage()
  {
    DoneStorage(EnableIf<ModeT>());
    sqlite3_finalize(m_prepared_statement);
    sqlite3_close(m_db);
  }

  void InitStorage(EnableIf<MODE_WRITE>)
  {
    string create_table("drop table if exists points; drop index if exists points_idx; create table points(id integer PRIMARY KEY, ll blob) WITHOUT ROWID;");
    if( sqlite3_exec(m_db, create_table.c_str(), NULL, NULL, NULL ) != SQLITE_OK ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(m_db));
      sqlite3_close(m_db);
      exit(1);
    }

    char* errorMessage;
    sqlite3_exec(m_db, "PRAGMA synchronous=OFF", NULL, NULL, &errorMessage);
    sqlite3_exec(m_db, "PRAGMA count_changes=OFF", NULL, NULL, &errorMessage);
    sqlite3_exec(m_db, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errorMessage);
    sqlite3_exec(m_db, "PRAGMA temp_store=MEMORY", NULL, NULL, &errorMessage);

    string insert("insert into points(id, ll) values(?,?);");
    if( sqlite3_prepare_v2(m_db, insert.c_str(), -1, &m_prepared_statement, NULL) != SQLITE_OK ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(m_db));
      sqlite3_close(m_db);
      exit(1);
    }
    if( sqlite3_exec(m_db, "BEGIN TRANSACTION", NULL, NULL, NULL ) != SQLITE_OK ){
      fprintf(stderr, "Can't start transaction: %s\n", sqlite3_errmsg(m_db));
      sqlite3_close(m_db);
      exit(1);
    }
  }

  void InitStorage(EnableIf<MODE_READ>)
  {
    string select("select ll from points where id=?;");
    if( sqlite3_prepare_v2(m_db, select.c_str(), -1, &m_prepared_statement, NULL) != SQLITE_OK ){
      fprintf(stderr, "failed sqlite3_prepare_v2: %s\n", sqlite3_errmsg(m_db));
      sqlite3_close(m_db);
      exit(1);
    }
  }

  void DoneStorage(EnableIf<MODE_WRITE>)
  {
    if( sqlite3_exec(m_db, "COMMIT TRANSACTION", NULL, NULL, NULL ) != SQLITE_OK ){
      fprintf(stderr, "Can't end transaction: %s\n", sqlite3_errmsg(m_db));
      sqlite3_close(m_db);
      exit(1);
    }
    if( sqlite3_exec(m_db, "create unique index points_idx on points(id);", NULL, NULL, NULL ) != SQLITE_OK ){
      fprintf(stderr, "Can't end transaction: %s\n", sqlite3_errmsg(m_db));
      sqlite3_close(m_db);
      exit(1);
    }
  }

  void DoneStorage(EnableIf<MODE_READ>) {}

  void AddPoint(uint64_t id, double lat, double lng)
  {
    LatLon ll = {lat, lng};

    if (sqlite3_bind_int64(m_prepared_statement, 1, id) != SQLITE_OK) {
      cerr << "bind1 failed: " << sqlite3_errmsg(m_db) << endl;
      exit(1);
    }
    if (sqlite3_bind_blob(m_prepared_statement, 2, &ll, sizeof(ll), SQLITE_STATIC) != SQLITE_OK) {
      cerr << "bind2 failed: " << sqlite3_errmsg(m_db) << endl;
      exit(1);
    }

    if (sqlite3_step(m_prepared_statement) != SQLITE_DONE) {
      cerr << "execution failed: " << sqlite3_errmsg(m_db) << endl;
      exit(1);
    }

    sqlite3_reset(m_prepared_statement);
    m_progress.Inc();
  }

  bool GetPoint(uint64_t id, double & lat, double & lng) const
  {
    bool found = true;
    if (sqlite3_bind_int64(m_prepared_statement, 1, id) != SQLITE_OK) {
      cerr << "bind1 failed: " << sqlite3_errmsg(m_db) << endl;
      exit(1);
    }
    int rc = sqlite3_step(m_prepared_statement);
    if (rc == SQLITE_DONE)
    {
      found = false;
    }
    else if (rc == SQLITE_ROW)
    {
      const void * data;
      data = sqlite3_column_blob (m_prepared_statement, 0);
      LatLon const &ll = *((LatLon const *)data);
      lat = ll.lat;
      lng = ll.lon;
    }
    else
    {
      cerr << "execution failed: " << sqlite3_errmsg(m_db) << endl;
      exit(1);
    }

    sqlite3_reset(m_prepared_statement);
    return found;
  }

};

template < BasePointStorage::EStorageMode ModeT >
class RawFilePointStorage : public BasePointStorage
{

#ifdef OMIM_OS_WINDOWS
  typedef FileReader FileReaderT;
#else
  typedef MmapReader FileReaderT;
#endif

  typename std::conditional<ModeT, FileWriter, FileReaderT>::type m_file;

public:
  RawFilePointStorage(string const & name) : BasePointStorage(name, 1000), m_file(name) {}

  template <bool T = (ModeT == BasePointStorage::MODE_WRITE)>
  typename std::enable_if<T, void>::type AddPoint(uint64_t id, double lat, double lng)
  {
    LatLon ll;
    ll.lat = lat;
    ll.lon = lng;
    m_file.Seek(id * sizeof(ll));
    m_file.Write(&ll, sizeof(ll));

    m_progress.Inc();
  }

  template <bool T = (ModeT == BasePointStorage::MODE_READ)>
  typename std::enable_if<T, bool>::type GetPoint(uint64_t id, double & lat, double & lng) const
  {
    // I think, it's not good idea to write this ugly code.
    // memcpy isn't to slow for that.
    //#ifdef OMIM_OS_WINDOWS
    LatLon ll;
    m_file.Read(id * sizeof(ll), &ll, sizeof(ll));
    //#else
    //    LatLon const & ll = *reinterpret_cast<LatLon const *>(m_file.Data() + id * sizeof(ll));
    //#endif

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

  typename std::conditional<ModeT, FileWriter, FileReaderT>::type m_file;

  double const m_precision = 10000000;

public:
  RawFileShortPointStorage(string const & name) : BasePointStorage(name, 1000), m_file(name) {}

  template <bool T = (ModeT == BasePointStorage::MODE_WRITE)>
  typename std::enable_if<T, void>::type AddPoint(uint64_t id, double lat, double lng)
  {
    int64_t const lat64 = lat * m_precision;
    int64_t const lng64 = lng * m_precision;

    ShortLatLon ll;
    ll.lat = static_cast<int32_t>(lat64);
    ll.lon = static_cast<int32_t>(lng64);
    CHECK_EQUAL(static_cast<int64_t>(ll.lat), lat64, ("Latitude is out of 32bit boundary!"));
    CHECK_EQUAL(static_cast<int64_t>(ll.lon), lng64, ("Longtitude is out of 32bit boundary!"));

    m_file.Seek(id * sizeof(ll));
    m_file.Write(&ll, sizeof(ll));

    m_progress.Inc();
  }

  template <bool T = (ModeT == BasePointStorage::MODE_READ)>
  typename std::enable_if<T, bool>::type GetPoint(uint64_t id, double & lat, double & lng) const
  {
    // I think, it's not good idea to write this ugly code.
    // memcpy isn't to slow for that.
    //#ifdef OMIM_OS_WINDOWS
    ShortLatLon ll;
    m_file.Read(id * sizeof(ll), &ll, sizeof(ll));
    //#else
    //    LatLon const & ll = *reinterpret_cast<LatLon const *>(m_file.Data() + id * sizeof(ll));
    //#endif

    // assume that valid coordinate is not (0, 0)
    if (ll.lat != 0.0 || ll.lon != 0.0)
    {
      lat = static_cast<double>(ll.lat) / m_precision;
      lng = static_cast<double>(ll.lon) / m_precision;
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
  typename std::conditional<ModeT, FileWriter, FileReader>::type m_file;


  typedef unordered_map<uint64_t, pair<double, double> > ContainerT;
  ContainerT m_map;

public:
  MapFilePointStorage(string const & name) : BasePointStorage(name, 10000), m_file(name)
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

      (void)m_map.insert(make_pair(ll.pos, make_pair(ll.lat, ll.lon)));

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
    
    m_progress.Inc();
  }

  bool GetPoint(uint64_t id, double & lat, double & lng) const
  {
    auto i = m_map.find(id);
    if (i != m_map.end())
    {
      lat = i->second.first;
      lng = i->second.second;
      return true;
    }
    return false;
  }

};

template < BasePointStorage::EStorageMode ModeT >
class MapFileShortPointStorage : public BasePointStorage
{
  typename std::conditional<ModeT, FileWriter, FileReader>::type m_file;


  typedef unordered_map<uint64_t, pair<int32_t, int32_t> > ContainerT;
  ContainerT m_map;

  double const m_precision = 10000000;

public:
  MapFileShortPointStorage(string const & name) : BasePointStorage(name, 10000), m_file(name+".short")
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

      (void)m_map.insert(make_pair(ll.pos, make_pair(ll.lat, ll.lon)));

      pos += sizeof(ll);
    }

    LOG(LINFO, ("Nodes reading is finished"));
  }

  void AddPoint(uint64_t id, double lat, double lng)
  {

    int64_t const lat64 = lat * m_precision;
    int64_t const lng64 = lng * m_precision;



    ShortLatLonPos ll;
    ll.pos = id;
    ll.lat = static_cast<int32_t>(lat64);
    ll.lon = static_cast<int32_t>(lng64);
    CHECK_EQUAL(static_cast<int64_t>(ll.lat), lat64, ("Latitude is out of 32bit boundary!"));
    CHECK_EQUAL(static_cast<int64_t>(ll.lon), lng64, ("Longtitude is out of 32bit boundary!"));
    m_file.Write(&ll, sizeof(ll));

    m_progress.Inc();
  }

  bool GetPoint(uint64_t id, double & lat, double & lng) const
  {
    auto i = m_map.find(id);
    if (i != m_map.end())
    {
      lat = static_cast<double>(i->second.first) / m_precision;
      lng = static_cast<double>(i->second.second) / m_precision;
      return true;
    }
    return false;
  }
  
};


