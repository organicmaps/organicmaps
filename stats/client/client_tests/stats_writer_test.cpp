#include "../../testing/testing.hpp"

#include "../stats_writer.hpp"
#include "../leveldb_reader.hpp"

#include "../../coding/file_writer.hpp"

#include "../../std/cstdlib.hpp"
#include "../../std/target_os.hpp"

#include <city.h>

#define USER_ID "123"
#define DB_PATH "tmp_testdb"

using namespace stats;

namespace
{

template<class P>
bool Compare(P const & a, P const & b)
{
  string as, bs;
  a.SerializeToString(&as);
  b.SerializeToString(&bs);
  return as == bs;
}

template<class P>
string Diff(P const & act, P const & exp)
{
//  Doesn't work with lite protos.
//  return string("\nactual: ") + act.DebugString() +
//    + " expect: " + exp.DebugString();
  return "";
}

UNIT_TEST(Simple)
{
  Search s;
  s.set_query("pizza nyc");

  {
    StatsWriter w(USER_ID, DB_PATH);
    TEST(w.Write(s), ());
  }

  vector<Event> v = ReadAllFromLevelDB<Event>(DB_PATH);
  TEST_EQUAL(v.size(), 1, ());

  Event exp;
  exp.MutableExtension(Search::event)->CopyFrom(s);
  exp.set_userid(CityHash64(USER_ID, strlen(USER_ID)));
  exp.set_timestamp(0);

  Event act = v[0];
  act.set_timestamp(0);

  TEST(Compare(act, exp), (Diff(act, exp)));

#ifdef OMIM_OS_WINDOWS
  system("rmdir /s /q " DB_PATH);
#else
  system("rm -rf " DB_PATH);
#endif
}

}  // namespace
