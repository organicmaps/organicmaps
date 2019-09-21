#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

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

  struct LessChunks
  {
    bool operator() (ChunkT const & r1, ChunkT const & r2) const { return r1.m_pos < r2.m_pos; }
    bool operator() (ChunkT const & r1, int64_t const & r2) const { return r1.m_pos < r2; }
    bool operator() (int64_t const & r1, ChunkT const & r2) const { return r1 < r2.m_pos; }
  };

  using RangeT = std::pair<int64_t, int64_t>;

  static const int SERVER_READY = -1;
  struct ServerT
  {
    std::string m_url;
    int m_chunkIndex;

    ServerT(std::string const & url, int ind) : m_url(url), m_chunkIndex(ind) {}
  };

  std::vector<ChunkT> m_chunks;

  std::vector<ServerT> m_servers;

  /// @return Chunk pointer and it's index for given file offsets range.
  std::pair<ChunkT *, int> GetChunk(RangeT const & range);

public:
  ChunksDownloadStrategy(std::vector<std::string> const & urls);

  /// Init chunks vector for fileSize.
  void InitChunks(int64_t fileSize, int64_t chunkSize, ChunkStatusT status = CHUNK_FREE);

  /// Used in unit tests only!
  void AddChunk(RangeT const & range, ChunkStatusT status);

  void SaveChunks(int64_t fileSize, std::string const & fName);
  /// @return Already downloaded size.
  int64_t LoadOrInitChunks(std::string const & fName, int64_t fileSize, int64_t chunkSize);

  /// Should be called for every completed chunk (no matter successful or not).
  /// @returns url of the chunk
  std::string ChunkFinished(bool success, RangeT const & range);

  size_t ActiveServersCount() const { return m_servers.size(); }

  enum ResultT
  {
    ENextChunk,
    ENoFreeServers,
    EDownloadFailed,
    EDownloadSucceeded
  };
  /// Should be called until returns ENextChunk
  ResultT NextChunk(std::string & outUrl, RangeT & range);
};
} // namespace downloader
