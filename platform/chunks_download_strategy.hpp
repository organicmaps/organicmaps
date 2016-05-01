#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/utility.hpp"
#include "std/cstdint.hpp"


namespace downloader
{

/// Single-threaded code
class ChunksDownloadStrategy
{
public:
  enum ChunkStatusT { CHUNK_FREE = 0, CHUNK_DOWNLOADING = 1, CHUNK_COMPLETE = 2, CHUNK_AUX = -1 };

private:
#pragma pack(push, 1)
  struct ChunkT
  {
    /// position of chunk in file
    int64_t m_pos;
    /// @see ChunkStatusT
    int8_t m_status;

    ChunkT() : m_pos(-1), m_status(-1)
    {
      static_assert(sizeof(ChunkT) == 9, "Be sure to avoid overhead in writing to file.");
    }
    ChunkT(int64_t pos, int8_t st) : m_pos(pos), m_status(st) {}
  };
#pragma pack(pop)

  vector<ChunkT> m_chunks;

  static const int SERVER_READY = -1;
  struct ServerT
  {
    string m_url;
    int m_chunkIndex;

    ServerT(string const & url, int ind) : m_url(url), m_chunkIndex(ind) {}
  };

  vector<ServerT> m_servers;

  struct LessChunks
  {
    bool operator() (ChunkT const & r1, ChunkT const & r2) const { return r1.m_pos < r2.m_pos; }
    bool operator() (ChunkT const & r1, int64_t const & r2) const { return r1.m_pos < r2; }
    bool operator() (int64_t const & r1, ChunkT const & r2) const { return r1 < r2.m_pos; }
  };

  typedef pair<int64_t, int64_t> RangeT;

  /// @return Chunk pointer and it's index for given file offsets range.
  pair<ChunkT *, int> GetChunk(RangeT const & range);

public:
  ChunksDownloadStrategy(vector<string> const & urls);

  /// Init chunks vector for fileSize.
  void InitChunks(int64_t fileSize, int64_t chunkSize, ChunkStatusT status = CHUNK_FREE);

  /// Used in unit tests only!
  void AddChunk(RangeT const & range, ChunkStatusT status);

  void SaveChunks(int64_t fileSize, string const & fName);
  /// @return Already downloaded size.
  int64_t LoadOrInitChunks(string const & fName, int64_t fileSize, int64_t chunkSize);

  /// Should be called for every completed chunk (no matter successful or not).
  /// @returns url of the chunk
  string ChunkFinished(bool success, RangeT const & range);

  size_t ActiveServersCount() const { return m_servers.size(); }

  enum ResultT
  {
    ENextChunk,
    ENoFreeServers,
    EDownloadFailed,
    EDownloadSucceeded
  };
  /// Should be called until returns ENextChunk
  ResultT NextChunk(string & outUrl, RangeT & range);
};

} // namespace downloader
