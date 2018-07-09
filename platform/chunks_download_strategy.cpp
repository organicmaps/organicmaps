#include "platform/chunks_download_strategy.hpp"

#include "coding/file_writer.hpp"
#include "coding/file_reader.hpp"
#include "coding/varint.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"


namespace downloader
{

ChunksDownloadStrategy::ChunksDownloadStrategy(vector<string> const & urls)
{
  // init servers list
  for (size_t i = 0; i < urls.size(); ++i)
    m_servers.push_back(ServerT(urls[i], SERVER_READY));
}

pair<ChunksDownloadStrategy::ChunkT *, int>
ChunksDownloadStrategy::GetChunk(RangeT const & range)
{
  vector<ChunkT>::iterator i = lower_bound(m_chunks.begin(), m_chunks.end(), range.first, LessChunks());

  if (i != m_chunks.end() && i->m_pos == range.first)
  {
    ASSERT_EQUAL ( (i+1)->m_pos, range.second + 1, () );
    return pair<ChunkT *, int>(&(*i), distance(m_chunks.begin(), i));
  }
  else
  {
    LOG(LERROR, ("Downloader error. Invalid chunk range: ", range));
    return pair<ChunkT *, int>(static_cast<ChunkT *>(0), -1);
  }
}

void ChunksDownloadStrategy::InitChunks(int64_t fileSize, int64_t chunkSize, ChunkStatusT status)
{
  m_chunks.reserve(fileSize / chunkSize + 2);
  for (int64_t i = 0; i < fileSize; i += chunkSize)
    m_chunks.push_back(ChunkT(i, status));
  m_chunks.push_back(ChunkT(fileSize, CHUNK_AUX));
}

void ChunksDownloadStrategy::AddChunk(RangeT const & range, ChunkStatusT status)
{
  ASSERT_LESS_OR_EQUAL ( range.first, range.second, () );
  if (m_chunks.empty())
  {
    ASSERT_EQUAL ( range.first, 0, () );
    m_chunks.push_back(ChunkT(range.first, status));
  }
  else
  {
    ASSERT_EQUAL ( m_chunks.back().m_pos, range.first, () );
    m_chunks.back().m_status = status;
  }

  m_chunks.push_back(ChunkT(range.second + 1, CHUNK_AUX));
}

void ChunksDownloadStrategy::SaveChunks(int64_t fileSize, string const & fName)
{
  if (!m_chunks.empty())
  {
    try
    {
      FileWriter w(fName);
      WriteVarInt(w, fileSize);

      w.Write(&m_chunks[0], sizeof(ChunkT) * m_chunks.size());
      return;
    }
    catch (FileWriter::Exception const & e)
    {
      LOG(LWARNING, ("Can't save chunks to file", e.Msg()));
    }
  }

  // Delete if no chunks or some error occured.
  (void)FileWriter::DeleteFileX(fName);
}

int64_t ChunksDownloadStrategy::LoadOrInitChunks(string const & fName, int64_t fileSize,
                                                 int64_t chunkSize)
{
  ASSERT ( fileSize > 0, () );
  ASSERT ( chunkSize > 0, () );

  try
  {
    FileReader r(fName);
    ReaderSource<FileReader> src(r);

    int64_t const readSize = ReadVarInt<int64_t>(src);
    if (readSize == fileSize)
    {
      // Load chunks.
      uint64_t const size = src.Size();
      int const stSize = sizeof(ChunkT);
      size_t const count = size / stSize;
      ASSERT_EQUAL(size, stSize * count, ());

      m_chunks.resize(count);
      src.Read(&m_chunks[0], stSize * count);

      // Reset status "downloading" to "free".
      int64_t downloadedSize = 0;
      for (size_t i = 0; i < count-1; ++i)
      {
        if (m_chunks[i].m_status != CHUNK_COMPLETE)
          m_chunks[i].m_status = CHUNK_FREE;
        else
          downloadedSize += (m_chunks[i+1].m_pos - m_chunks[i].m_pos);
      }

      return downloadedSize;
    }
  }
  catch (FileReader::Exception const & e)
  {
    LOG(LDEBUG, (e.Msg()));
  }

  InitChunks(fileSize, chunkSize);
  return 0;
}

string ChunksDownloadStrategy::ChunkFinished(bool success, RangeT const & range)
{
  pair<ChunkT *, int> res = GetChunk(range);
  string url;
  // find server which was downloading this chunk
  if (res.first)
  {
    for (size_t s = 0; s < m_servers.size(); ++s)
    {
      if (m_servers[s].m_chunkIndex == res.second)
      {
        url = m_servers[s].m_url;
        if (success)
        {
          // mark server as free and chunk as ready
          m_servers[s].m_chunkIndex = SERVER_READY;
          res.first->m_status = CHUNK_COMPLETE;
        }
        else
        {
          LOG(LINFO, ("Thread for url", m_servers[s].m_url,
                      "failed to download chunk number", m_servers[s].m_chunkIndex));
          // remove failed server and mark chunk as free
          m_servers.erase(m_servers.begin() + s);
          res.first->m_status = CHUNK_FREE;
        }
        break;
      }
    }
  }
  return url;
}

ChunksDownloadStrategy::ResultT
ChunksDownloadStrategy::NextChunk(string & outUrl, RangeT & range)
{
  // If no servers at all.
  if (m_servers.empty())
    return EDownloadFailed;

  // Find first free server.
  ServerT * server = 0;
  for (size_t i = 0; i < m_servers.size(); ++i)
  {
    if (m_servers[i].m_chunkIndex == SERVER_READY)
    {
      server = &m_servers[i];
      break;
    }
  }
  if (server == 0)
    return ENoFreeServers;

  bool allChunksDownloaded = true;

  // Find first free chunk.
  for (size_t i = 0; i < m_chunks.size()-1; ++i)
  {
    switch (m_chunks[i].m_status)
    {
    case CHUNK_FREE:
      server->m_chunkIndex = static_cast<int>(i);
      outUrl = server->m_url;

      range.first = m_chunks[i].m_pos;
      range.second = m_chunks[i+1].m_pos - 1;

      m_chunks[i].m_status = CHUNK_DOWNLOADING;
      return ENextChunk;

    case CHUNK_DOWNLOADING:
      allChunksDownloaded = false;
      break;
    }
  }

  return (allChunksDownloaded ? EDownloadSucceeded : ENoFreeServers);
}

} // namespace downloader
