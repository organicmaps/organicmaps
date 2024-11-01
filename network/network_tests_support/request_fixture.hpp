#pragma once

#include "base/logging.hpp"

#include <QtWidgets/QApplication>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "network/http/request.hpp"

namespace om::network::testing
{
using ::testing::IsEmpty;
using ::testing::Not;

char constexpr kTestUrl1[] = "http://localhost:34568/unit_tests/1.txt";
char constexpr kTestUrl404[] = "http://localhost:34568/unit_tests/notexisting_unittest";
char constexpr kTestUrlBigFile[] = "http://localhost:34568/unit_tests/47kb.file";

class RequestFixture : public ::testing::Test
{
public:
  RequestFixture()
    : m_app{[]()
            {
              int args = 0;
              return QApplication(args, nullptr);
            }()}
  {
  }

  auto MakeGetRequest(std::string const & url)
  {
    return http::Request::Get(
        url, [this](http::Request & request) { OnDownloadFinish(request); },
        [this](http::Request & request) { OnDownloadProgress(request); });
  }

  auto MakePostRequest(std::string const & url, std::string const & data)
  {
    return http::Request::PostJson(
        url, data, [this](http::Request & request) { OnDownloadFinish(request); },
        [this](http::Request & request) { OnDownloadProgress(request); });
  }

  auto MakeFileRequest(std::vector<std::string> const & urls, std::string const & fileName, int64_t fileSize,
                       int64_t chunkSize = 512 * 1024, bool doCleanOnCancel = true)
  {
    return http::Request::GetFile(
        urls, fileName, fileSize, [this](http::Request & request) { OnDownloadFinish(request); },
        [this](http::Request & request) { OnDownloadProgress(request); }, chunkSize);
  }

  void ExpectOK()
  {
    EXPECT_THAT(m_statuses, Not(IsEmpty())) << "Observer was not called";
    EXPECT_TRUE(m_progressWasCalled) << "Download progress wasn't called";
    for (auto const & status : m_statuses)
      EXPECT_EQ(status, DownloadStatus::Completed);
  }

  void ExpectFail()
  {
    EXPECT_THAT(m_statuses, Not(IsEmpty())) << "Observer was not called";
    for (auto const & status : m_statuses)
      EXPECT_EQ(status, DownloadStatus::Failed);
  }

  void ExpectFileNotFound()
  {
    EXPECT_THAT(m_statuses, Not(IsEmpty())) << "Observer was not called.";
    for (auto const & status : m_statuses)
      EXPECT_EQ(status, DownloadStatus::FileNotFound);
  }

protected:
  void CancelDownloadOnGivenChunk(int chunksToFail) { m_chunksToFail = chunksToFail; }

private:
  void OnDownloadProgress(http::Request & request)
  {
    m_progressWasCalled = true;
    EXPECT_EQ(request.GetStatus(), DownloadStatus::InProgress);

    // Cancel download if needed
    if (m_chunksToFail != -1)
    {
      --m_chunksToFail;
      if (m_chunksToFail == 0)
      {
        m_chunksToFail = -1;
        LOG(LINFO, ("Download canceled"));
        QCoreApplication::quit();
      }
    }
  }

  void OnDownloadFinish(http::Request & request)
  {
    m_statuses.emplace_back(request.GetStatus());
    EXPECT_NE(m_statuses.back(), DownloadStatus::InProgress);
    QCoreApplication::quit();
  }

protected:
  bool m_progressWasCalled = false;
  /// Chunked downloads can return one status per chunk (thread)
  std::vector<DownloadStatus> m_statuses;
  /// Interrupt download after this number of chunks
  int m_chunksToFail = -1;
  base::ScopedLogLevelChanger const m_debugLogLevel{LDEBUG};
  QApplication m_app;
};
}  // namespace om::network::testing
