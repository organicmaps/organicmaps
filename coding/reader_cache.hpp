#pragma once

#include "base/base.hpp"
#include "base/cache.hpp"
#include "base/stats.hpp"

#include "std/algorithm.hpp"
#include "std/cstring.hpp"
#include "std/sstream.hpp"
#include "std/vector.hpp"


namespace impl
{

template <bool bEnable> struct ReaderCacheStats
{
  string GetStatsStr(uint32_t, uint32_t) const { return ""; }
  my::NoopStats<uint32_t> m_ReadSize;
  my::NoopStats<uint32_t> m_CacheHit;
};

template <> struct ReaderCacheStats<true>
{
  string GetStatsStr(uint32_t logPageSize, uint32_t pageCount) const
  {
    ostringstream out;
    out << "LogPageSize: " << logPageSize << " PageCount: " << pageCount;
    out << " ReadSize(" << m_ReadSize.GetStatsStr() << ")";
    out << " CacheHit(" << m_CacheHit.GetStatsStr() << ")";
    double const bytesAsked = m_ReadSize.GetAverage() * m_ReadSize.GetCount();
    double const callsMade = (1.0 - m_CacheHit.GetAverage()) * m_CacheHit.GetCount();
    double const bytesRead = callsMade * (1 << logPageSize);
    out << " RatioBytesRead: " << (bytesRead + 1) / (bytesAsked + 1);
    out << " RatioCallsMade: " << (callsMade + 1) / (m_ReadSize.GetCount() + 1);
    return out.str();
  }

  my::AverageStats<uint32_t> m_ReadSize;
  my::AverageStats<uint32_t> m_CacheHit;
};

}

template <class ReaderT, bool bStats = false>
class ReaderCache
{
public:
  ReaderCache(uint32_t logPageSize, uint32_t logPageCount)
    : m_Cache(logPageCount), m_LogPageSize(logPageSize)
  {
  }

  void Read(ReaderT & reader, uint64_t pos, void * p, size_t size)
  {
    if (size == 0)
      return;
    ASSERT_LESS_OR_EQUAL(pos + size, reader.Size(), (pos, size, reader.Size()));
    m_Stats.m_ReadSize(static_cast<uint32_t>(size));
    char * pDst = static_cast<char *>(p);
    uint64_t pageNum = pos >> m_LogPageSize;
    size_t const firstPageOffset = static_cast<size_t>(pos - (pageNum << m_LogPageSize));
    size_t const firstCopySize = min(size, PageSize() - firstPageOffset);
    ASSERT_GREATER(firstCopySize, 0, ());
    memcpy(pDst, ReadPage(reader, pageNum) + firstPageOffset, firstCopySize);
    size -= firstCopySize;
    pos += firstCopySize;
    pDst += firstCopySize;
    ++pageNum;
    while (size > 0)
    {
      size_t const copySize = min(size, PageSize());
      memcpy(pDst, ReadPage(reader, pageNum), copySize);
      size -= copySize;
      pos += copySize;
      pDst += copySize;
      ++pageNum;
    }
  }

  string GetStatsStr() const
  {
    return m_Stats.GetStatsStr(m_LogPageSize, m_Cache.GetCacheSize());
  }

private:
  inline size_t PageSize() const { return 1 << m_LogPageSize; }

  inline char const * ReadPage(ReaderT & reader, uint64_t pageNum)
  {
    bool cached;
    vector<char> & v = m_Cache.Find(pageNum, cached);
    m_Stats.m_CacheHit(cached ? 1 : 0);
    if (!cached)
    {
      if (v.empty())
        v.resize(PageSize());
      uint64_t const pos = pageNum << m_LogPageSize;
      reader.Read(pos, &v[0], min(PageSize(), static_cast<size_t>(reader.Size() - pos)));
    }
    return &v[0];
  }

  my::Cache<uint64_t, vector<char> > m_Cache;
  uint32_t const m_LogPageSize;
  impl::ReaderCacheStats<bStats> m_Stats;
};
