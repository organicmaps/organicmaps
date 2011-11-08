#include "../../testing/testing.hpp"

#include "../../base/logging.hpp"

#include "../../coding/file_reader.hpp"
#include "../../coding/file_writer.hpp"
#include "../../coding/sha2.hpp"

#include "../http_request.hpp"
#include "../chunks_download_strategy.hpp"

#include "../../std/scoped_ptr.hpp"
#include "../../std/bind.hpp"

#include <QCoreApplication>

#define TEST_URL1 "http://melnichek.ath.cx:34568/unit_tests/1.txt"
#define TEST_URL_404 "http://melnichek.ath.cx:34568/unit_tests/notexisting_unittest"
#define TEST_URL_PERMANENT "http://melnichek.ath.cx:34568/unit_tests/permanent"
#define TEST_URL_INVALID_HOST "http://melnichek12345.ath.cx"
#define TEST_URL_BIG_FILE "http://melnichek.ath.cx:34568/unit_tests/47kb.file"
#define TEST_URL_HTTPS "https://github.com"
#define TEST_URL_POST "http://melnichek.ath.cx:34568/unit_tests/post.php"

using namespace downloader;


class DownloadObserver
{
  bool m_progressWasCalled;
  HttpRequest::StatusT * m_status;

public:
  DownloadObserver() : m_status(0)
  {
    Reset();
    my::g_LogLevel = LDEBUG;
  }

  void Reset()
  {
    m_progressWasCalled = false;
    if (m_status)
      delete m_status;
    m_status = 0;
  }

  void TestOk()
  {
    TEST(m_progressWasCalled, ("Download progress wasn't called"));
    TEST_NOT_EQUAL(m_status, 0, ());
    TEST_EQUAL(*m_status, HttpRequest::ECompleted, ());
  }

  void TestFailed()
  {
    TEST_NOT_EQUAL(m_status, 0, ());
    TEST_EQUAL(*m_status, HttpRequest::EFailed, ());
  }

  void OnDownloadProgress(HttpRequest & request)
  {
    m_progressWasCalled = true;
    TEST_EQUAL(request.Status(), HttpRequest::EInProgress, ());
  }

  void OnDownloadFinish(HttpRequest & request)
  {
    TEST_EQUAL(m_status, 0, ());
    m_status = new HttpRequest::StatusT(request.Status());
    TEST(*m_status == HttpRequest::EFailed || *m_status == HttpRequest::ECompleted, ());
    QCoreApplication::quit();
  }

};

struct CancelDownload
{
  void OnProgress(HttpRequest & request)
  {
    TEST_GREATER(request.Data().size(), 0, ());
    delete &request;
    QCoreApplication::quit();
  }
  void OnFinish(HttpRequest &)
  {
    TEST(false, ("Should be never called"));
  }
};

struct DeleteOnFinish
{
  void OnProgress(HttpRequest & request)
  {
    TEST_GREATER(request.Data().size(), 0, ());
  }
  void OnFinish(HttpRequest & request)
  {
    delete &request;
    QCoreApplication::quit();
  }
};

UNIT_TEST(DownloaderSimpleGet)
{
  DownloadObserver observer;
  HttpRequest::CallbackT onFinish = bind(&DownloadObserver::OnDownloadFinish, &observer, _1);
  HttpRequest::CallbackT onProgress = bind(&DownloadObserver::OnDownloadProgress, &observer, _1);
  { // simple success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL1, string(), onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data(), "Test1", ());
  }

  observer.Reset();
  { // permanent redirect success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_PERMANENT, string(), onFinish, onProgress));
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data(), "Test1", ());
  }

  observer.Reset();
  { // fail case 404
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_404, string(), onFinish, onProgress));
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(request->Data().size(), 0, (request->Data()));
  }

  observer.Reset();
  { // fail case not existing host
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_INVALID_HOST, string(), onFinish, onProgress));
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(request->Data().size(), 0, (request->Data()));
  }

  { // cancel download in the middle of the progress
    CancelDownload canceler;
    /// should be deleted in canceler
    HttpRequest::Get(TEST_URL_BIG_FILE, string(),
        bind(&CancelDownload::OnFinish, &canceler, _1),
        bind(&CancelDownload::OnProgress, &canceler, _1));
    QCoreApplication::exec();
  }

  observer.Reset();
  { // https success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_HTTPS, string(), onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_GREATER(request->Data().size(), 0, ());
  }

  string const FILENAME = "some_downloader_test_file";
  observer.Reset();
  { // download file success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL1, FILENAME, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    {
      FileReader f(FILENAME);
      string s;
      f.ReadAsString(s);
      TEST_EQUAL(s, "Test1", ());
    }
    TEST_EQUAL(request->Data(), FILENAME, (request->Data()));
    FileWriter::DeleteFileX(FILENAME);
  }

  observer.Reset();
  { // download file error case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_404, FILENAME, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestFailed();
    {
      FileReader f(FILENAME);
      TEST_EQUAL(f.Size(), 0, ());
    }
    TEST_EQUAL(request->Data(), FILENAME, (request->Data()));
    FileWriter::DeleteFileX(FILENAME);
  }

  { // Delete request at the end of successful file download
    DeleteOnFinish deleter;
    /// should be deleted in deleter on finish
    HttpRequest::Get(TEST_URL1, FILENAME,
        bind(&DeleteOnFinish::OnFinish, &deleter, _1),
        bind(&DeleteOnFinish::OnProgress, &deleter, _1));
    QCoreApplication::exec();
    {
      FileReader f(FILENAME);
      string s;
      f.ReadAsString(s);
      TEST_EQUAL(s, "Test1", ());
    }
    FileWriter::DeleteFileX(FILENAME);
  }
}

UNIT_TEST(DownloaderSimplePost)
{
  DownloadObserver observer;
  HttpRequest::CallbackT onFinish = bind(&DownloadObserver::OnDownloadFinish, &observer, _1);
  HttpRequest::CallbackT onProgress = bind(&DownloadObserver::OnDownloadProgress, &observer, _1);
  { // simple success case
    string const postData = "{\"jsonKey\":\"jsonValue\"}";
    scoped_ptr<HttpRequest> request(HttpRequest::Post(TEST_URL_POST, string(), postData, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data(), postData, ());
  }
}

UNIT_TEST(ChunksDownloadStrategy)
{
  string const S1 = "UrlOfServer1";
  string const S2 = "UrlOfServer2";
  string const S3 = "UrlOfServer3";
  pair<int64_t, int64_t> const R1(0, 249), R2(250, 499), R3(500, 749), R4(750, 800);
  vector<string> servers;
  servers.push_back(S1);
  servers.push_back(S2);
  servers.push_back(S3);
  int64_t const FILE_SIZE = 800;
  int64_t const CHUNK_SIZE = 250;
  ChunksDownloadStrategy strategy(servers, FILE_SIZE, CHUNK_SIZE);

  string s1;
  int64_t beg1, end1;
  TEST_EQUAL(strategy.NextChunk(s1, beg1, end1), ChunksDownloadStrategy::ENextChunk, ());

  string s2;
  int64_t beg2, end2;
  TEST_EQUAL(strategy.NextChunk(s2, beg2, end2), ChunksDownloadStrategy::ENextChunk, ());

  string s3;
  int64_t beg3, end3;
  TEST_EQUAL(strategy.NextChunk(s3, beg3, end3), ChunksDownloadStrategy::ENextChunk, ());

  string sEmpty;
  int64_t begEmpty, endEmpty;
  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  TEST(s1 != s2 && s2 != s3 && s3 != s1, (s1, s2, s3));

  pair<int64_t, int64_t> const r1(beg1, end1), r2(beg2, end2), r3(beg3, end3);
  TEST(r1 != r2 && r2 != r3 && r3 != r1, (r1, r2, r3));
  TEST(r1 == R1 || r1 == R2 || r1 == R3 || r1 == R4, (r1));
  TEST(r2 == R1 || r2 == R2 || r2 == R3 || r2 == R4, (r2));
  TEST(r3 == R1 || r3 == R2 || r3 == R3 || r3 == R4, (r3));

  strategy.ChunkFinished(true, beg1, end1);

  string s4;
  int64_t beg4, end4;
  TEST_EQUAL(strategy.NextChunk(s4, beg4, end4), ChunksDownloadStrategy::ENextChunk, ());
  TEST_EQUAL(s4, s1, ());
  pair<int64_t, int64_t> const r4(beg4, end4);
  TEST(r4 != r1 && r4 != r2 && r4 != r3, (r4));

  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(false, beg2, end2);

  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(true, beg4, end4);

  string s5;
  int64_t beg5, end5;
  TEST_EQUAL(strategy.NextChunk(s5, beg5, end5), ChunksDownloadStrategy::ENextChunk, ());
  TEST_EQUAL(s5, s4, (s5, s4));
  TEST(beg5 == beg2 && end5 == end2, ());

  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(true, beg5, end5);

  // 3rd is still alive here
  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(true, beg3, end3);

  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::EDownloadSucceeded, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, begEmpty, endEmpty), ChunksDownloadStrategy::EDownloadSucceeded, ());
}

UNIT_TEST(ChunksDownloadStrategyFAIL)
{
  string const S1 = "UrlOfServer1";
  string const S2 = "UrlOfServer2";
  pair<int64_t, int64_t> const R1(0, 249), R2(250, 499), R3(500, 749), R4(750, 800);
  vector<string> servers;
  servers.push_back(S1);
  servers.push_back(S2);
  int64_t const FILE_SIZE = 800;
  int64_t const CHUNK_SIZE = 250;
  ChunksDownloadStrategy strategy(servers, FILE_SIZE, CHUNK_SIZE);

  string s1;
  int64_t beg1, end1;
  TEST_EQUAL(strategy.NextChunk(s1, beg1, end1), ChunksDownloadStrategy::ENextChunk, ());
  string s2;
  int64_t beg2, end2;
  TEST_EQUAL(strategy.NextChunk(s2, beg2, end2), ChunksDownloadStrategy::ENextChunk, ());
  TEST_EQUAL(strategy.NextChunk(s2, beg2, end2), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(false, beg1, end1);

  TEST_EQUAL(strategy.NextChunk(s2, beg2, end2), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(false, beg2, end2);

  TEST_EQUAL(strategy.NextChunk(s2, beg2, end2), ChunksDownloadStrategy::EDownloadFailed, ());
}

UNIT_TEST(DownloadChunks)
{
  DownloadObserver observer;
  HttpRequest::CallbackT onFinish = bind(&DownloadObserver::OnDownloadFinish, &observer, _1);
  HttpRequest::CallbackT onProgress = bind(&DownloadObserver::OnDownloadProgress, &observer, _1);

  vector<string> urls;
  urls.push_back(TEST_URL1);
  urls.push_back(TEST_URL1);
  int64_t FILESIZE = 5;

  { // should use only one thread
    scoped_ptr<HttpRequest> request(HttpRequest::GetChunks(urls, string(), FILESIZE, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data().size(), FILESIZE, ());
    TEST_EQUAL(request->Data(), "Test1", ());
  }

  observer.Reset();

  urls.clear();
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  FILESIZE = 5;

  { // 3 threads - fail, because of invalid size
    scoped_ptr<HttpRequest> request(HttpRequest::GetChunks(urls, string(), FILESIZE, onFinish, onProgress, 2048));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(request->Data().size(), 0, ());
  }

  string const SHA256 = "EE6AE6A2A3619B2F4A397326BEC32583DE2196D9D575D66786CB3B6F9D04A633";

  observer.Reset();

  urls.clear();
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  FILESIZE = 47684;

  { // 3 threads - succeeded
    scoped_ptr<HttpRequest> request(HttpRequest::GetChunks(urls, string(), FILESIZE, onFinish, onProgress, 2048));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data().size(), FILESIZE, ());
    TEST_EQUAL(sha2::digest256(request->Data()), SHA256, ());
  }

  observer.Reset();

  urls.clear();
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL1);
  urls.push_back(TEST_URL_404);
  FILESIZE = 47684;

  { // 3 threads with only one valid url - succeeded
    scoped_ptr<HttpRequest> request(HttpRequest::GetChunks(urls, string(), FILESIZE, onFinish, onProgress, 2048));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data().size(), FILESIZE, ());
    TEST_EQUAL(sha2::digest256(request->Data()), SHA256, ());
  }

  observer.Reset();

  urls.clear();
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  FILESIZE = 12345;

  { // 2 threads and all points to file with invalid size - fail
    scoped_ptr<HttpRequest> request(HttpRequest::GetChunks(urls, string(), FILESIZE, onFinish, onProgress, 2048));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(request->Data().size(), 0, ());
  }

  observer.Reset();

  urls.clear();
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  FILESIZE = 47684;
  string const FILENAME = "some_downloader_test_file";

  { // 3 threads - download to file - succeeded
    scoped_ptr<HttpRequest> request(HttpRequest::GetChunks(urls, FILENAME, FILESIZE, onFinish, onProgress, 10000));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data(), FILENAME, ());
    {
      FileReader f(FILENAME);
      TEST_EQUAL(f.Size(), FILESIZE, ());
      string s;
      f.ReadAsString(s);
      TEST_EQUAL(sha2::digest256(s), SHA256, ());
    }
    FileWriter::DeleteFileX(FILENAME);
  }
}
