#include "chunks_download_strategy.hpp"

#include "../std/algorithm.hpp"

#define INVALID_CHUNK -1


namespace downloader
{

ChunksDownloadStrategy::RangeT const ChunksDownloadStrategy::INVALID_RANGE = RangeT(-1, -1);

ChunksDownloadStrategy::ChunksDownloadStrategy(vector<string> const & urls, int64_t fileSize, int64_t chunkSize)
  : m_chunkSize(chunkSize)
{
  // init servers list
  for (size_t i = 0; i < urls.size(); ++i)
    m_servers.push_back(make_pair(urls[i], INVALID_RANGE));

  // init chunks which should be downloaded
  // @TODO implement download resume by saving chunks to download for specified file
  for (int64_t i = 0; i < fileSize; i += chunkSize)
  {
    m_chunksToDownload.insert(RangeT(i, min(i + chunkSize - 1,
                                            fileSize - 1)));
  }
}

void ChunksDownloadStrategy::ChunkFinished(bool successfully, int64_t begRange, int64_t endRange)
{
  RangeT const chunk(begRange, endRange);
  // find server which was downloading this chunk
  for (ServersT::iterator it = m_servers.begin(); it != m_servers.end(); ++it)
  {
    if (it->second == chunk)
    {
      if (successfully)
        it->second = INVALID_RANGE;
      else
      {
        // remove failed server and mark chunk as not downloaded
        m_servers.erase(it);
        m_chunksToDownload.insert(chunk);
      }
      break;
    }
  }
}

ChunksDownloadStrategy::ResultT ChunksDownloadStrategy::NextChunk(string & outUrl,
                                                                  int64_t & begRange,
                                                                  int64_t & endRange)
{
  if (m_servers.empty())
    return EDownloadFailed;

  if (m_chunksToDownload.empty())
  {
    // no more chunks to download
    bool allChunksAreFinished = true;
    for (size_t i = 0; i < m_servers.size(); ++i)
    {
      if (m_servers[i].second != INVALID_RANGE)
        allChunksAreFinished = false;
    }
    if (allChunksAreFinished)
      return EDownloadSucceeded;
    else
      return ENoFreeServers;
  }
  else
  {
    RangeT const nextChunk = *m_chunksToDownload.begin();
    for (size_t i = 0; i < m_servers.size(); ++i)
    {
      if (m_servers[i].second == INVALID_RANGE)
      {
        // found not used server
        m_servers[i].second = nextChunk;
        outUrl = m_servers[i].first;
        begRange = nextChunk.first;
        endRange = nextChunk.second;
        m_chunksToDownload.erase(m_chunksToDownload.begin());
        return ENextChunk;
      }
    }
    // if we're here, all servers are busy downloading
    return ENoFreeServers;
  }
}

} // namespace downloader
