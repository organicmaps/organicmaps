#include "network/network_tests_support/request_fixture.hpp"

namespace om::network::downloader::testing
{
using namespace ::testing;
using namespace om::network::testing;

namespace
{
struct CancelDownload
{
  static void OnDownloadProgress(http::Request & request)
  {
    EXPECT_THAT(request.GetData(), Not(IsEmpty()));
    delete &request;
    QCoreApplication::quit();
  }

  static void OnDownloadFinish(http::Request &) { ASSERT_TRUE(false) << "Should be never called"; }
};

struct DeleteOnFinish
{
  static void OnDownloadProgress(http::Request & request) { EXPECT_THAT(request.GetData(), Not(IsEmpty())); }

  static void OnDownloadFinish(http::Request & request)
  {
    delete &request;
    QCoreApplication::quit();
  }
};
}  // namespace

class SimpleDownloaderTests : public RequestFixture
{
public:
  using RequestFixture::MakeGetRequest;

  template <class Functor>
  auto MakeGetRequest(std::string const & url)
  {
    return http::Request::Get(
        url, [](http::Request & request) { Functor::OnDownloadFinish(request); },
        [](http::Request & request) { Functor::OnDownloadProgress(request); });
  }
};

TEST_F(SimpleDownloaderTests, SimpleHttpGetSuccess)
{
  std::unique_ptr<http::Request> const request{MakeGetRequest(kTestUrl1)};
  // wait until download is finished
  QCoreApplication::exec();
  ExpectOK();
  EXPECT_EQ(request->GetData(), "Test1");
}

TEST_F(SimpleDownloaderTests, SimpleHttpsGetSuccess)
{
  std::unique_ptr<http::Request> const request{MakeGetRequest("https://github.com")};
  // wait until download is finished
  QCoreApplication::exec();
  ExpectOK();
  EXPECT_THAT(request->GetData(), Not(IsEmpty()));
}

TEST_F(SimpleDownloaderTests, FailOnRedirect)
{
  // We DO NOT SUPPORT redirects to avoid data corruption when downloading mwm files
  std::unique_ptr<http::Request> const request{MakeGetRequest("http://localhost:34568/unit_tests/permanent")};
  // wait until download is finished
  QCoreApplication::exec();
  ExpectFail();
  EXPECT_THAT(request->GetData(), IsEmpty());
}

TEST_F(SimpleDownloaderTests, FailWith404)
{
  std::unique_ptr<http::Request> const request{MakeGetRequest(kTestUrl404)};
  // wait until download is finished
  QCoreApplication::exec();
  ExpectFileNotFound();
  EXPECT_THAT(request->GetData(), IsEmpty());
}

TEST_F(SimpleDownloaderTests, FailWhenNotExistingHost)
{
  std::unique_ptr<http::Request> const request{MakeGetRequest("http://not-valid-host123532.ath.cx")};
  // wait until download is finished
  QCoreApplication::exec();
  ExpectFail();
  EXPECT_THAT(request->GetData(), IsEmpty());
}

TEST_F(SimpleDownloaderTests, CancelDownloadInTheMiddleOfTheProgress)
{
  // should be deleted in canceler
  MakeGetRequest<CancelDownload>(kTestUrlBigFile);
  QCoreApplication::exec();
}

TEST_F(SimpleDownloaderTests, DeleteRequestAtTheEndOfSuccessfulDownload)
{
  // should be deleted in deleter on finish
  MakeGetRequest<DeleteOnFinish>(kTestUrl1);
  QCoreApplication::exec();
}

TEST_F(SimpleDownloaderTests, SimplePostRequest)
{
  std::string const postData = R"({"jsonKey":"jsonValue"})";
  std::unique_ptr<http::Request> const request{MakePostRequest("http://localhost:34568/unit_tests/post.php", postData)};
  // wait until post is finished
  QCoreApplication::exec();
  ExpectOK();
  EXPECT_EQ(request->GetData(), postData);
}
}  // namespace om::network::downloader::testing
