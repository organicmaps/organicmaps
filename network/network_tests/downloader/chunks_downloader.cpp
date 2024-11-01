#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "network/chunks_download_strategy.hpp"
#include "network/network_tests_support/request_fixture.hpp"

namespace om::network::downloader::testing
{
using namespace ::testing;
using namespace om::network::testing;

namespace
{
// Should match file size in tools/python/ResponseProvider.py
int constexpr kBigFileSize = 47684;
}  // namespace

class ChunksDownloaderTests : public RequestFixture
{
public:
  void TearDown() override { DeleteTempDownloadedFiles(); }

  using RequestFixture::MakeFileRequest;

  template <class Functor>
  auto MakeFileRequest(std::vector<std::string> const & urls, std::string const & fileName, int64_t fileSize,
                       Functor & f)
  {
    return http::Request::GetFile(
        urls, fileName, fileSize, [&f](http::Request & request) { f.OnDownloadFinish(request); },
        [&f](http::Request & request) { f.OnDownloadProgress(request); });
  }

  void ExpectFileDownloadSuccessAndClear()
  {
    ExpectOK();

    EXPECT_TRUE(base::DeleteFileX(m_fileName)) << "Result file should present on success";

    uint64_t size;
    EXPECT_FALSE(base::GetFileSize(m_fileName + DOWNLOADING_FILE_EXTENSION, size)) << "No downloading file on success";
    EXPECT_FALSE(base::GetFileSize(m_fileName + RESUME_FILE_EXTENSION, size)) << "No resume file on success";
  }

  void ExpectFileDownloadFailAndClear()
  {
    ExpectFail();

    uint64_t size;
    EXPECT_FALSE(base::GetFileSize(m_fileName, size)) << "No result file on fail";
    UNUSED_VALUE(base::DeleteFileX(m_fileName + DOWNLOADING_FILE_EXTENSION));
    EXPECT_TRUE(base::DeleteFileX(m_fileName + RESUME_FILE_EXTENSION)) << "Resume file should present on fail";
  }

protected:
  static std::string ReadFileAsString(std::string const & file)
  {
    try
    {
      FileReader f(file);
      std::string s;
      f.ReadAsString(s);
      return s;
    }
    catch (FileReader::Exception const &)
    {
      EXPECT_TRUE(false) << "File " << file << " should exist";
      return "";
    }
  }

  void DeleteTempDownloadedFiles()
  {
    FileWriter::DeleteFileX(m_fileName + RESUME_FILE_EXTENSION);
    FileWriter::DeleteFileX(m_fileName + DOWNLOADING_FILE_EXTENSION);
  }

  static std::string GetFileNameForTest()
  {
    const auto testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    const std::string testSuiteName{testInfo->test_suite_name()};
    const std::string testName{testInfo->name()};
    return Platform().TmpPathForFile(testSuiteName + "_" + testName);
  }

  std::string m_fileName = GetFileNameForTest();
};

TEST_F(ChunksDownloaderTests, OneThreadSuccess)
{
  const std::vector<std::string> urls = {kTestUrl1, kTestUrl1};
  const int64_t fileSize = 5;

  // should use only one thread
  std::unique_ptr<http::Request> const request{MakeFileRequest(urls, m_fileName, fileSize, 512 * 1024)};
  // wait until download is finished
  QCoreApplication::exec();
  EXPECT_EQ(request->GetData(), m_fileName);
  EXPECT_EQ(ReadFileAsString(m_fileName), "Test1");
  ExpectFileDownloadSuccessAndClear();
}

TEST_F(ChunksDownloaderTests, ThreeThreadsSuccess)
{
  const std::vector<std::string> urls = {kTestUrlBigFile, kTestUrlBigFile, kTestUrlBigFile};
  const int64_t fileSize = kBigFileSize;

  // should use only one thread
  std::unique_ptr<http::Request> const request{MakeFileRequest(urls, m_fileName, fileSize, 2048)};
  UNUSED_VALUE(request);
  // wait until download is finished
  QCoreApplication::exec();
  ExpectFileDownloadSuccessAndClear();
}

TEST_F(ChunksDownloaderTests, ThreeThreadsFail)
{
  const std::vector<std::string> urls = {kTestUrlBigFile, kTestUrlBigFile, kTestUrlBigFile};
  const int64_t fileSize = 5;

  // should use only one thread
  std::unique_ptr<http::Request> const request{MakeFileRequest(urls, m_fileName, fileSize, 2048)};
  UNUSED_VALUE(request);
  // wait until download is finished
  QCoreApplication::exec();
  ExpectFileDownloadFailAndClear();
}

TEST_F(ChunksDownloaderTests, ThreeThreadsWithOnlyOneValidURLSuccess)
{
  const std::vector<std::string> urls = {kTestUrlBigFile, kTestUrl1, kTestUrl404};
  const int64_t fileSize = kBigFileSize;

  std::unique_ptr<http::Request> const request{MakeFileRequest(urls, m_fileName, fileSize, 2048)};
  UNUSED_VALUE(request);
  // wait until download is finished
  QCoreApplication::exec();
  ExpectFileDownloadSuccessAndClear();
}

TEST_F(ChunksDownloaderTests, TwoThreadsWithInvalidSizeFail)
{
  const std::vector<std::string> urls = {kTestUrlBigFile, kTestUrlBigFile};
  const int64_t fileSize = 12345;

  std::unique_ptr<http::Request> const request{MakeFileRequest(urls, m_fileName, fileSize, 2048)};
  UNUSED_VALUE(request);
  // wait until download is finished
  QCoreApplication::exec();
  ExpectFileDownloadFailAndClear();
}

namespace
{
int64_t constexpr beg1 = 123, end1 = 1230, beg2 = 44000, end2 = 47683;

struct ResumeChecker
{
  size_t m_counter;
  ResumeChecker() : m_counter(0) {}

  void OnDownloadProgress(http::Request & request)
  {
    if (m_counter == 0)
    {
      EXPECT_EQ(request.GetProgress().m_bytesDownloaded, beg2);
      EXPECT_EQ(request.GetProgress().m_bytesTotal, kBigFileSize);
    }
    else if (m_counter == 1)
    {
      EXPECT_EQ(request.GetProgress().m_bytesDownloaded, kBigFileSize);
      EXPECT_EQ(request.GetProgress().m_bytesTotal, kBigFileSize);
    }
    else
    {
      EXPECT_TRUE(false) << "Progress should be called exactly 2 times";
    }
    ++m_counter;
  }

  static void OnDownloadFinish(http::Request & request)
  {
    EXPECT_EQ(request.GetStatus(), DownloadStatus::Completed);
    QCoreApplication::exit();
  }
};
}  // namespace

TEST_F(ChunksDownloaderTests, DownloadChunksWithResume)
{
  std::vector<std::string> urls = {kTestUrlBigFile};

  // 1st step - download full file
  {
    std::unique_ptr<http::Request> const request(MakeFileRequest(urls, m_fileName, kBigFileSize));
    // wait until download is finished
    QCoreApplication::exec();
    ExpectOK();
    uint64_t size;
    EXPECT_FALSE(base::GetFileSize(m_fileName + RESUME_FILE_EXTENSION, size)) << "No resume file on success";
  }

  // 2nd step - mark some file blocks as not downloaded
  {
    // to substitute temporary not fully downloaded file
    EXPECT_TRUE(base::RenameFileX(m_fileName, m_fileName + DOWNLOADING_FILE_EXTENSION));

    FileWriter f(m_fileName + DOWNLOADING_FILE_EXTENSION, FileWriter::OP_WRITE_EXISTING);
    f.Seek(beg1);
    char b1[end1 - beg1 + 1] = {0};
    f.Write(b1, ARRAY_SIZE(b1));

    f.Seek(beg2);
    char b2[end2 - beg2 + 1] = {0};
    f.Write(b2, ARRAY_SIZE(b2));

    ChunksDownloadStrategy strategy;
    strategy.AddChunk({int64_t(0), beg1 - 1}, ChunksDownloadStrategy::CHUNK_COMPLETE);
    strategy.AddChunk({beg1, end1}, ChunksDownloadStrategy::CHUNK_FREE);
    strategy.AddChunk({end1 + 1, beg2 - 1}, ChunksDownloadStrategy::CHUNK_COMPLETE);
    strategy.AddChunk({beg2, end2}, ChunksDownloadStrategy::CHUNK_FREE);

    strategy.SaveChunks(kBigFileSize, m_fileName + RESUME_FILE_EXTENSION);
  }

  // 3rd step - check that resume works
  {
    ResumeChecker checker;
    std::unique_ptr<http::Request> const request(MakeFileRequest(urls, m_fileName, kBigFileSize, checker));
    QCoreApplication::exec();
    ExpectFileDownloadSuccessAndClear();
  }
}

TEST_F(ChunksDownloaderTests, DownloadChunksResumeAfterCancel)
{
  std::vector<std::string> urls = {kTestUrlBigFile};
  int arrCancelChunks[] = {1, 3, 10, 15, 20, 0};

  for (int arrCancelChunk : arrCancelChunks)
  {
    if (arrCancelChunk > 0)
      CancelDownloadOnGivenChunk(arrCancelChunk);

    std::unique_ptr<http::Request> const request(MakeFileRequest(urls, m_fileName, kBigFileSize, 1024, false));
    QCoreApplication::exec();
  }

  ExpectFileDownloadSuccessAndClear();
}
}  // namespace om::network::downloader::testing
