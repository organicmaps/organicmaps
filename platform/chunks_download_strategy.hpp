#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"
#include "../std/set.hpp"

namespace downloader
{

class ChunksDownloadStrategy
{
  int64_t m_chunkSize;

  typedef pair<int64_t, int64_t> RangeT;
  static RangeT const INVALID_RANGE;
  /// <server url, currently downloading range or INVALID_RANGE if url is not used>
  typedef vector<pair<string, RangeT> > ServersT;
  ServersT m_servers;
  set<RangeT> m_chunksToDownload;

public:
  ChunksDownloadStrategy(vector<string> const & urls, int64_t fileSize, int64_t chunkSize = 512 * 1024);

  void ChunkFinished(bool successfully, int64_t begRange, int64_t endRange);
  enum ResultT
  {
    ENextChunk,
    ENoFreeServers,
    EDownloadFailed,
    EDownloadSucceeded
  };
  ResultT NextChunk(string & outUrl, int64_t & begRange, int64_t & endRange);
};

} // namespace downloader
