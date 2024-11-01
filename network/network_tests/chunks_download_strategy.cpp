#include "network/chunks_download_strategy.hpp"

#include <gtest/gtest.h>

namespace om::network
{
using namespace ::testing;

using RangeT = std::pair<int64_t, int64_t>;

TEST(ChunksDownloadStrategy, Success)
{
  std::vector<std::string> const servers = {"UrlOfServer1", "UrlOfServer2", "UrlOfServer3"};

  RangeT const R1{0, 249}, R2{250, 499}, R3{500, 749}, R4{750, 800};

  int64_t constexpr kFileSize = 800;
  int64_t constexpr kChunkSize = 250;
  ChunksDownloadStrategy strategy(servers);
  strategy.InitChunks(kFileSize, kChunkSize);

  std::string s1;
  RangeT r1;
  EXPECT_EQ(strategy.NextChunk(s1, r1), ChunksDownloadStrategy::ENextChunk);

  std::string s2;
  RangeT r2;
  EXPECT_EQ(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENextChunk);

  std::string s3;
  RangeT r3;
  EXPECT_EQ(strategy.NextChunk(s3, r3), ChunksDownloadStrategy::ENextChunk);

  std::string sEmpty;
  RangeT rEmpty;
  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers);
  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers);

  EXPECT_TRUE(s1 != s2 && s2 != s3 && s3 != s1) << s1 << " " << s2 << " " << s3;

  EXPECT_TRUE(r1 != r2 && r2 != r3 && r3 != r1)
      << PrintToString(r1) << " " << PrintToString(r2) << " " << PrintToString(r3);
  EXPECT_TRUE(r1 == R1 || r1 == R2 || r1 == R3 || r1 == R4) << PrintToString(r1);
  EXPECT_TRUE(r2 == R1 || r2 == R2 || r2 == R3 || r2 == R4) << PrintToString(r2);
  EXPECT_TRUE(r3 == R1 || r3 == R2 || r3 == R3 || r3 == R4) << PrintToString(r3);

  strategy.ChunkFinished(true, r1);

  std::string s4;
  RangeT r4;
  EXPECT_EQ(strategy.NextChunk(s4, r4), ChunksDownloadStrategy::ENextChunk);
  EXPECT_EQ(s4, s1);
  EXPECT_TRUE(r4 != r1 && r4 != r2 && r4 != r3) << PrintToString(r4);

  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers);
  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers);

  strategy.ChunkFinished(false, r2);

  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers);

  strategy.ChunkFinished(true, r4);

  std::string s5;
  RangeT r5;
  EXPECT_EQ(strategy.NextChunk(s5, r5), ChunksDownloadStrategy::ENextChunk);
  EXPECT_EQ(s5, s4) << PrintToString(s5) << " " << PrintToString(s4);
  EXPECT_EQ(r5, r2);

  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers);
  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers);

  strategy.ChunkFinished(true, r5);

  // 3rd is still alive here
  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers);
  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers);

  strategy.ChunkFinished(true, r3);

  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::EDownloadSucceeded);
  EXPECT_EQ(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::EDownloadSucceeded);
}

TEST(ChunksDownloadStrategy, Fail)
{
  std::vector<std::string> const servers = {"UrlOfServer1", "UrlOfServer2"};

  int64_t constexpr kFileSize = 800;
  int64_t constexpr kChunkSize = 250;
  ChunksDownloadStrategy strategy(servers);
  strategy.InitChunks(kFileSize, kChunkSize);

  std::string s1;
  RangeT r1;
  EXPECT_EQ(strategy.NextChunk(s1, r1), ChunksDownloadStrategy::ENextChunk);

  std::string s2;
  RangeT r2;
  EXPECT_EQ(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENextChunk);
  EXPECT_EQ(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENoFreeServers);

  strategy.ChunkFinished(false, r1);

  EXPECT_EQ(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENoFreeServers);

  strategy.ChunkFinished(false, r2);

  EXPECT_EQ(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::EDownloadFailed);
}
}  // namespace om::network
