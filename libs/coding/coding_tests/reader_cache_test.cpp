#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/reader_cache.hpp"

#include <algorithm>
#include <random>
#include <string>
#include <vector>

using namespace std;

namespace
{
template <class ReaderT>
class CacheReader
{
public:
  CacheReader(ReaderT const & reader, uint32_t logPageSize, uint32_t logPageCount)
    : m_Reader(reader)
    , m_Cache(logPageSize, logPageCount)
  {}

  void Read(uint64_t pos, void * p, size_t size) const { m_Cache.Read(m_Reader, pos, p, size); }

private:
  ReaderT m_Reader;
  ReaderCache<ReaderT const> mutable m_Cache;
};
}  // namespace

UNIT_TEST(CacheReaderRandomTest)
{
  vector<char> data(100000);
  for (size_t i = 0; i < data.size(); ++i)
    data[i] = static_cast<char>(i % 253);
  MemReader memReader(&data[0], data.size());
  CacheReader<MemReader> cacheReader(MemReader(&data[0], data.size()), 10, 5);
  mt19937 rng(0);
  for (size_t i = 0; i < 100000; ++i)
  {
    size_t pos = rng() % data.size();
    size_t len = min(static_cast<size_t>(1 + (rng() % 127)), data.size() - pos);
    string readMem(len, '0'), readCache(len, '0');
    memReader.Read(pos, &readMem[0], len);
    cacheReader.Read(pos, &readCache[0], len);
    TEST_EQUAL(readMem, readCache, (pos, len, i));
  }
}
