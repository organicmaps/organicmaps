#include "testing/testing.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace http_client_test
{
using namespace platform;
using std::string;

char constexpr kServerUrl[] = "http://localhost:34568";
char constexpr kTestUrl1[] = "http://localhost:34568/unit_tests/1.txt";
char constexpr kTestUrl404[] = "http://localhost:34568/unit_tests/notexisting_unittest";
char constexpr kTestUrlBigFile[] = "http://localhost:34568/unit_tests/47kb.file";
char constexpr kTestUrlEchoHeaders[] = "http://localhost:34568/unit_tests/echo_headers";
char constexpr kTestUrlEchoCookies[] = "http://localhost:34568/unit_tests/echo_cookies";
char constexpr kTestUrlTimeout[] = "http://localhost:34568/unit_tests/timeout";
char constexpr kTestUrl500[] = "http://localhost:34568/unit_tests/500";
char constexpr kTestUrl403[] = "http://localhost:34568/unit_tests/403";
char constexpr kTestUrlRedirect[] = "http://localhost:34568/unit_tests/redirect_to_1txt";
char constexpr kTestUrlSetCookies[] = "http://localhost:34568/unit_tests/set_cookies";
char constexpr kTestUrlMultiSetCookies[] = "http://localhost:34568/unit_tests/multi_set_cookies";
char constexpr kTestUrlPost[] = "http://localhost:34568/unit_tests/post.php";

int constexpr kBigFileSize = 47684;

// Helper: block until async request completes and return the result.
HttpClient::Result RunAsync(HttpClient & client)
{
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  HttpClient::Result result;

  client.RunHttpRequestAsync([&](HttpClient::Result r)
  {
    std::lock_guard lock(mu);
    result = std::move(r);
    done = true;
    cv.notify_one();
  });

  std::unique_lock lock(mu);
  cv.wait(lock, [&] { return done; });
  return result;
}

// =====================================================================
// Sync wrapper: RunHttpRequest()
// =====================================================================

UNIT_TEST(HttpClient_SyncGet_Success)
{
  HttpClient client(kTestUrl1);
  TEST(client.RunHttpRequest(), ());
  TEST_EQUAL(client.ErrorCode(), 200, ());
  TEST_EQUAL(client.ServerResponse(), "Test1", ());
}

UNIT_TEST(HttpClient_SyncGet_404)
{
  HttpClient client(kTestUrl404);
  // 404 is still a valid HTTP response — RunHttpRequest should return true.
  TEST(client.RunHttpRequest(), ());
  TEST_EQUAL(client.ErrorCode(), 404, ());
}

UNIT_TEST(HttpClient_SyncGet_500)
{
  HttpClient client(kTestUrl500);
  TEST(client.RunHttpRequest(), ());
  TEST_EQUAL(client.ErrorCode(), 500, ());
  TEST_EQUAL(client.ServerResponse(), "Internal Server Error", ());
}

UNIT_TEST(HttpClient_SyncGet_403)
{
  HttpClient client(kTestUrl403);
  TEST(client.RunHttpRequest(), ());
  TEST_EQUAL(client.ErrorCode(), 403, ());
}

UNIT_TEST(HttpClient_SyncGet_InvalidHost)
{
  HttpClient client("http://very-invalid-host-that-does-not-exist-12345.example");
  client.SetTimeout(5);
  TEST(!client.RunHttpRequest(), ());
}

UNIT_TEST(HttpClient_SyncGetWithResponse)
{
  HttpClient client(kTestUrl1);
  string response;
  TEST(client.RunHttpRequest(response), ());
  TEST_EQUAL(response, "Test1", ());
}

UNIT_TEST(HttpClient_SyncGetWithResponse_CustomChecker)
{
  HttpClient client(kTestUrl404);
  string response;
  auto checker = [](HttpClient const & r) { return r.ErrorCode() == 404; };
  TEST(client.RunHttpRequest(response, checker), ());
}

UNIT_TEST(HttpClient_SyncGetWithResponse_FailsDefaultChecker)
{
  HttpClient client(kTestUrl404);
  string response;
  // Default checker expects 200 — 404 should fail.
  TEST(!client.RunHttpRequest(response), ());
  TEST(response.empty(), ());
}

// =====================================================================
// Async: RunHttpRequestAsync()
// =====================================================================

UNIT_TEST(HttpClient_AsyncGet_Success)
{
  HttpClient client(kTestUrl1);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 200, ());
  TEST_EQUAL(result.m_serverResponse, "Test1", ());
}

UNIT_TEST(HttpClient_AsyncGet_404)
{
  HttpClient client(kTestUrl404);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 404, ());
}

UNIT_TEST(HttpClient_AsyncGet_500)
{
  HttpClient client(kTestUrl500);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 500, ());
}

UNIT_TEST(HttpClient_AsyncGet_InvalidHost)
{
  HttpClient client("http://very-invalid-host-that-does-not-exist-12345.example");
  client.SetTimeout(5);
  auto result = RunAsync(client);
  TEST(!result.m_success, ());
}

// HttpClient can be destroyed immediately after RunHttpRequestAsync returns.
UNIT_TEST(HttpClient_AsyncGet_ClientDestroyedBeforeCompletion)
{
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  HttpClient::Result result;

  {
    HttpClient client(kTestUrl1);
    client.RunHttpRequestAsync([&](HttpClient::Result r)
    {
      std::lock_guard lock(mu);
      result = std::move(r);
      done = true;
      cv.notify_one();
    });
    // client destroyed here
  }

  std::unique_lock lock(mu);
  cv.wait(lock, [&] { return done; });
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 200, ());
}

// =====================================================================
// Cancellation via RequestHandle
// =====================================================================

UNIT_TEST(HttpClient_Cancel_BeforeCompletion)
{
  HttpClient client(kTestUrlBigFile);
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  HttpClient::Result result;

  auto handle = client.RunHttpRequestAsync([&](HttpClient::Result r)
  {
    std::lock_guard lock(mu);
    result = std::move(r);
    done = true;
    cv.notify_one();
  });

  // Cancel immediately.
  handle.Cancel();
  TEST(handle.IsCancelled(), ());

  std::unique_lock lock(mu);
  cv.wait(lock, [&] { return done; });
  // Should get kCancelled or the request may have completed before cancel took effect.
  // Either outcome is acceptable — the key invariant is that the handler was called.
  TEST(result.m_errorCode == HttpClient::kCancelled || result.m_errorCode == 200, ());
}

UNIT_TEST(HttpClient_RequestHandle_DefaultNotCancelled)
{
  HttpClient::RequestHandle handle;
  TEST(!handle.IsCancelled(), ());
}

// =====================================================================
// POST with body data
// =====================================================================

UNIT_TEST(HttpClient_SyncPost_BodyData)
{
  string const body = R"({"key":"value"})";
  HttpClient client(kTestUrlPost);
  client.SetBodyData(body, "application/json");
  TEST(client.RunHttpRequest(), ());
  // The test server echoes back POST body.
  TEST_EQUAL(client.ServerResponse(), body, ());
}

UNIT_TEST(HttpClient_AsyncPost_BodyData)
{
  string const body = R"({"hello":"world"})";
  HttpClient client(kTestUrlPost);
  client.SetBodyData(body, "application/json");
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_serverResponse, body, ());
}

UNIT_TEST(HttpClient_Post_BodyFile)
{
  // Write a temp file, POST it via SetBodyFile, verify the server echoes it back.
  string const filePath = GetPlatform().TmpPathForFile("http_client_test_bodyfile_", ".tmp");
  string const body = R"({"from":"file"})";
  {
    FileWriter writer(filePath);
    writer.Write(body.data(), body.size());
  }
  HttpClient client(kTestUrlPost);
  client.SetBodyFile(filePath, "application/json");
  auto result = RunAsync(client);
  TEST(result.m_success, ("errorCode:", result.m_errorCode));
  TEST_EQUAL(result.m_serverResponse, body, ());
  base::DeleteFileX(filePath);
}

UNIT_TEST(HttpClient_Post_BodyFile_InvalidPath)
{
  HttpClient client(kTestUrlPost);
  client.SetBodyFile("nonexistent_dir_12345/missing.tmp", "application/json");
  auto result = RunAsync(client);
  TEST(!result.m_success, ());
}

// =====================================================================
// Custom headers
// =====================================================================

UNIT_TEST(HttpClient_SetRawHeader)
{
  HttpClient client(kTestUrlEchoHeaders);
  client.SetRawHeader("X-Custom-Test", "hello123");
  client.LoadHeaders(true);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  // The echo_headers endpoint returns headers as JSON.
  TEST(result.m_serverResponse.find("x-custom-test") != string::npos, (result.m_serverResponse));
  TEST(result.m_serverResponse.find("hello123") != string::npos, (result.m_serverResponse));
}

UNIT_TEST(HttpClient_SetRawHeaders_Multiple)
{
  HttpClient client(kTestUrlEchoHeaders);
  HttpClient::Headers headers;
  headers["X-First"] = "one";
  headers["X-Second"] = "two";
  client.SetRawHeaders(headers);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST(result.m_serverResponse.find("one") != string::npos, (result.m_serverResponse));
  TEST(result.m_serverResponse.find("two") != string::npos, (result.m_serverResponse));
}

// =====================================================================
// Cookies
// =====================================================================

UNIT_TEST(HttpClient_SetCookies)
{
  HttpClient client(kTestUrlEchoCookies);
  client.SetCookies("session=abc; user=test");
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST(result.m_serverResponse.find("session=abc") != string::npos, (result.m_serverResponse));
  TEST(result.m_serverResponse.find("user=test") != string::npos, (result.m_serverResponse));
}

UNIT_TEST(HttpClient_LoadHeaders_SetCookie)
{
  HttpClient client(kTestUrlSetCookies);
  client.LoadHeaders(true);
  TEST(client.RunHttpRequest(), ());
  auto const & headers = client.GetHeaders();
  auto it = headers.find("set-cookie");
  TEST(it != headers.end(), ());
  TEST(it->second.find("session=abc123") != string::npos, (it->second));
}

UNIT_TEST(HttpClient_CombinedCookies)
{
  HttpClient client(kTestUrlSetCookies);
  client.SetCookies("existing=cookie");
  // CombinedCookies() looks for "Set-Cookie" (mixed case).
  // LoadHeaders(true) lowercases all keys, so use LoadHeaders(false)
  // which stores Set-Cookie with the canonical mixed-case key.
  client.LoadHeaders(false);
  TEST(client.RunHttpRequest(), ());

  string combined = client.CombinedCookies();
  // Should contain both server-set and client-set cookies.
  TEST(combined.find("session=abc123") != string::npos, (combined));
  TEST(combined.find("existing=cookie") != string::npos, (combined));
}

// =====================================================================
// Redirects
// =====================================================================

UNIT_TEST(HttpClient_FollowRedirects_True)
{
  HttpClient client(kTestUrlRedirect);
  client.SetFollowRedirects(true);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 200, ());
  TEST_EQUAL(result.m_serverResponse, "Test1", ());
  // URL should be the final destination.
  TEST(result.m_urlReceived.find("1.txt") != string::npos, (result.m_urlReceived));
}

UNIT_TEST(HttpClient_FollowRedirects_False)
{
  HttpClient client(kTestUrlRedirect);
  client.SetFollowRedirects(false);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 301, ());
}

UNIT_TEST(HttpClient_WasRedirected)
{
  HttpClient client(kTestUrlRedirect);
  client.SetFollowRedirects(true);
  TEST(client.RunHttpRequest(), ());
  TEST(client.WasRedirected(), ());
}

UNIT_TEST(HttpClient_NotRedirected)
{
  HttpClient client(kTestUrl1);
  TEST(client.RunHttpRequest(), ());
  TEST(!client.WasRedirected(), ());
}

// =====================================================================
// Range requests
// =====================================================================

UNIT_TEST(HttpClient_RangeRequest_Partial)
{
  HttpClient client(kTestUrlBigFile);
  client.SetRange(0, 99);
  client.LoadHeaders(true);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 206, ());
  TEST_EQUAL(static_cast<int64_t>(result.m_serverResponse.size()), 100, ());

  // Verify Content-Range header.
  auto it = result.m_headers.find("content-range");
  TEST(it != result.m_headers.end(), ());
  TEST(it->second.find(strings::to_string(kBigFileSize)) != string::npos, (it->second));
}

UNIT_TEST(HttpClient_RangeRequest_OpenEnded)
{
  HttpClient client(kTestUrl1);
  // Request from byte 2 to end. "Test1" = 5 bytes, so we get "st1" (3 bytes).
  client.SetRange(2);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 206, ());
  TEST_EQUAL(result.m_serverResponse, "st1", ());
}

// =====================================================================
// Timeout
// =====================================================================

UNIT_TEST(HttpClient_Timeout)
{
  HttpClient client(kTestUrlTimeout);
  client.SetTimeout(2);  // 2 second timeout, server sleeps 60s.
  auto const start = std::chrono::steady_clock::now();
  TEST(!client.RunHttpRequest(), ());
  auto const elapsed = std::chrono::steady_clock::now() - start;
  // Should return within ~5 seconds (generous margin for CI).
  TEST(elapsed < std::chrono::seconds(15), ());
}

// =====================================================================
// ReceivedFile (download to file)
// =====================================================================

UNIT_TEST(HttpClient_ReceivedFile_Success)
{
  string const filePath = GetPlatform().TmpPathForFile("http_client_test_", ".tmp");
  {
    HttpClient client(kTestUrl1);
    client.SetReceivedFile(filePath);
    auto result = RunAsync(client);
    TEST(result.m_success, ("errorCode:", result.m_errorCode));
    TEST_EQUAL(result.m_errorCode, 200, ());
    // When writing to file, m_serverResponse should be empty.
    TEST(result.m_serverResponse.empty(), ());
  }
  // Verify file contents.
  {
    FileReader reader(filePath);
    string data;
    reader.ReadAsString(data);
    TEST_EQUAL(data, "Test1", ());
  }
  base::DeleteFileX(filePath);
}

UNIT_TEST(HttpClient_ReceivedFile_InvalidPath)
{
  // Path to a directory that doesn't exist — file open should fail.
  string const filePath = "nonexistent_dir_12345/http_test.tmp";
  HttpClient client(kTestUrl1);
  client.SetReceivedFile(filePath);
  auto result = RunAsync(client);

  // All platforms must report failure when the output file can't be created.
  TEST(!result.m_success, ());
  TEST_EQUAL(result.m_errorCode, HttpClient::kWriteException, ());
}

UNIT_TEST(HttpClient_ReceivedFile_WithProgress)
{
  // First, download to memory to get the reference body.
  string expectedBody;
  {
    HttpClient memClient(kTestUrlBigFile);
    auto memResult = RunAsync(memClient);
    TEST(memResult.m_success, ());
    expectedBody = std::move(memResult.m_serverResponse);
    TEST_GREATER(expectedBody.size(), 0, ());
  }

  // Now download to file with ProgressHandler and compare.
  string const filePath = GetPlatform().TmpPathForFile("http_client_test_progress_", ".tmp");
  std::atomic<bool> progressCalled{false};
  {
    HttpClient client(kTestUrlBigFile);
    client.SetReceivedFile(filePath);
    client.SetProgressHandler([&progressCalled](int64_t, int64_t)
    { progressCalled.store(true, std::memory_order_release); });
    auto result = RunAsync(client);
    TEST(result.m_success, ("errorCode:", result.m_errorCode));
    TEST_EQUAL(result.m_errorCode, 200, ());
    TEST(result.m_serverResponse.empty(), ());
  }
  {
    FileReader reader(filePath);
    string data;
    reader.ReadAsString(data);
    TEST_EQUAL(data.size(), expectedBody.size(), ());
    TEST_EQUAL(data, expectedBody, ());
  }
  TEST(progressCalled.load(std::memory_order_acquire), ());
  base::DeleteFileX(filePath);
}

UNIT_TEST(HttpClient_ReceivedFile_WithProgress_InvalidPath)
{
  string const filePath = "nonexistent_dir_12345/http_test.tmp";
  HttpClient client(kTestUrl1);
  client.SetReceivedFile(filePath);
  client.SetProgressHandler([](int64_t, int64_t) {});
  auto result = RunAsync(client);

  // Delegate path must also report write failure.
  TEST(!result.m_success, ());
  TEST_EQUAL(result.m_errorCode, HttpClient::kWriteException, ());
}

// =====================================================================
// LoadHeaders
// =====================================================================

UNIT_TEST(HttpClient_LoadHeaders_True)
{
  HttpClient client(kTestUrl1);
  client.LoadHeaders(true);
  TEST(client.RunHttpRequest(), ());
  auto const & headers = client.GetHeaders();
  // Server should send Content-Length at minimum.
  TEST(!headers.empty(), ());
}

UNIT_TEST(HttpClient_LoadHeaders_False_StillGetsCookies)
{
  HttpClient client(kTestUrlSetCookies);
  client.LoadHeaders(false);
  TEST(client.RunHttpRequest(), ());
  auto const & headers = client.GetHeaders();
  // Even without LoadHeaders, Set-Cookie should be captured.
  auto it = headers.find("Set-Cookie");
  TEST(it != headers.end(), ());
}

// =====================================================================
// HTTP methods
// =====================================================================

UNIT_TEST(HttpClient_HttpMethod_HEAD)
{
  HttpClient client(kTestUrl1);
  client.SetHttpMethod("HEAD");
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  // HEAD returns headers but no body.
  TEST(result.m_serverResponse.empty(), (result.m_serverResponse));
}

UNIT_TEST(HttpClient_HttpMethod_PUT)
{
  // PUT to the echo endpoint (test server echoes POST/PUT body for >= 300 routes).
  HttpClient client(kTestUrlPost);
  string const body = "put-data";
  client.SetBodyData(body, "text/plain", "PUT");
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_serverResponse, body, ());
}

// =====================================================================
// DataHandler (streaming)
// =====================================================================

UNIT_TEST(HttpClient_DataHandler_AccumulatesData)
{
  string accumulated;
  HttpClient client(kTestUrl1);
  client.SetDataHandler([&accumulated](void const * buf, size_t size) -> bool
  {
    accumulated.append(static_cast<char const *>(buf), size);
    return true;
  });
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  // Data went through the handler, not m_serverResponse.
  TEST(result.m_serverResponse.empty(), ());
  TEST_EQUAL(accumulated, "Test1", ());
}

UNIT_TEST(HttpClient_DataHandler_Abort)
{
  HttpClient client(kTestUrlBigFile);
  std::atomic<int> callCount{0};
  client.SetDataHandler([&callCount](void const *, size_t) -> bool
  {
    ++callCount;
    return false;  // Abort after first chunk.
  });
  auto result = RunAsync(client);
  // The request should have been aborted.
  TEST_EQUAL(callCount.load(), 1, ());
  // Aborted request must not report success.
  TEST(!result.m_success, ());
}

// =====================================================================
// ProgressHandler
// =====================================================================

UNIT_TEST(HttpClient_ProgressHandler_Called)
{
  std::atomic<bool> progressCalled{false};
  HttpClient client(kTestUrlBigFile);
  client.SetProgressHandler([&progressCalled](int64_t bytesReceived, int64_t totalBytes)
  { progressCalled.store(true, std::memory_order_release); });
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST(progressCalled.load(std::memory_order_acquire), ());
}

// =====================================================================
// Concurrent async requests
// =====================================================================

UNIT_TEST(HttpClient_ConcurrentRequests)
{
  int constexpr kNumRequests = 10;
  std::mutex mu;
  std::condition_variable cv;
  int completedCount = 0;
  HttpClient::Result results[kNumRequests];

  for (int i = 0; i < kNumRequests; ++i)
  {
    HttpClient client(kTestUrl1);
    client.RunHttpRequestAsync([&, i](HttpClient::Result r)
    {
      std::lock_guard lock(mu);
      results[i] = std::move(r);
      ++completedCount;
      if (completedCount == kNumRequests)
        cv.notify_one();
    });
  }

  std::unique_lock lock(mu);
  cv.wait(lock, [&] { return completedCount == kNumRequests; });

  for (int i = 0; i < kNumRequests; ++i)
  {
    TEST(results[i].m_success, ("Request", i, "failed"));
    TEST_EQUAL(results[i].m_errorCode, 200, ("Request", i));
    TEST_EQUAL(results[i].m_serverResponse, "Test1", ("Request", i));
  }
}

// =====================================================================
// NormalizeServerCookies (pure unit test, no server needed)
// =====================================================================

UNIT_TEST(HttpClient_NormalizeServerCookies)
{
  // Simple case.
  TEST_EQUAL(HttpClient::NormalizeServerCookies("a=1; Path=/"), "a=1", ());
  // Multiple cookies comma-separated.
  TEST_EQUAL(HttpClient::NormalizeServerCookies("a=1; Path=/, b=2; Path=/"), "a=1; b=2", ());
  // Ignore expires with comma in date.
  TEST_EQUAL(HttpClient::NormalizeServerCookies("a=1; expires=Thu, 01 Jan 2099 00:00:00 GMT, b=2"), "a=1; b=2", ());
}

// =====================================================================
// Platform-specific behavior checks
// =====================================================================

#if defined(OMIM_OS_LINUX) || defined(OMIM_OS_WINDOWS)
// Qt-specific: verify m_success is true for HTTP error responses (regression test).
UNIT_TEST(HttpClient_Qt_SuccessSemantics_HttpErrors)
{
  // Verify that HTTP 4xx/5xx responses still set m_success=true.
  // This was a regression where Qt's QNetworkReply::error() != NoError for HTTP errors.
  {
    HttpClient client(kTestUrl404);
    auto result = RunAsync(client);
    TEST(result.m_success, ("Qt must report m_success=true for 404"));
    TEST_EQUAL(result.m_errorCode, 404, ());
  }
  {
    HttpClient client(kTestUrl500);
    auto result = RunAsync(client);
    TEST(result.m_success, ("Qt must report m_success=true for 500"));
    TEST_EQUAL(result.m_errorCode, 500, ());
  }
  {
    HttpClient client(kTestUrl403);
    auto result = RunAsync(client);
    TEST(result.m_success, ("Qt must report m_success=true for 403"));
    TEST_EQUAL(result.m_errorCode, 403, ());
  }
}
#endif

#if defined(OMIM_OS_ANDROID)
// Android-specific: verify OkHttp async callback works.
UNIT_TEST(HttpClient_Android_AsyncCallback)
{
  HttpClient client(kTestUrl1);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 200, ());
  TEST_EQUAL(result.m_serverResponse, "Test1", ());
}
#endif

// =====================================================================
// Edge cases
// =====================================================================

UNIT_TEST(HttpClient_EmptyUrl)
{
  HttpClient client("");
  TEST(!client.RunHttpRequest(), ());
}

UNIT_TEST(HttpClient_UrlReceived_NoRedirect)
{
  HttpClient client(kTestUrl1);
  TEST(client.RunHttpRequest(), ());
  // Final URL should match the requested URL (possibly with trailing slash normalization).
  TEST(!client.UrlReceived().empty(), ());
}

UNIT_TEST(HttpClient_FluentApi_Chaining)
{
  HttpClient client(kTestUrl1);
  // Verify fluent API returns *this and is chainable.
  auto & ref = client.SetHttpMethod("GET")
                   .SetTimeout(10)
                   .SetFollowRedirects(true)
                   .SetRawHeader("Accept", "text/plain")
                   .SetCookies("test=1");
  TEST_EQUAL(&ref, &client, ());
  TEST(client.RunHttpRequest(), ());
  TEST_EQUAL(client.ErrorCode(), 200, ());
}

UNIT_TEST(HttpClient_LargeFileDownload)
{
  HttpClient client(kTestUrlBigFile);
  auto result = RunAsync(client);
  TEST(result.m_success, ());
  TEST_EQUAL(result.m_errorCode, 200, ());
  // The test server generates (BIG_FILE_SIZE + 1) * 2 bytes, trimmed to BIG_FILE_SIZE + 1.
  TEST_GREATER(result.m_serverResponse.size(), 0, ());
}

// Cancel a request that has likely already completed — should not crash.
UNIT_TEST(HttpClient_Cancel_AfterCompletion)
{
  HttpClient client(kTestUrl1);
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  auto handle = client.RunHttpRequestAsync([&](HttpClient::Result)
  {
    std::lock_guard lock(mu);
    done = true;
    cv.notify_one();
  });

  {
    std::unique_lock lock(mu);
    cv.wait(lock, [&] { return done; });
  }

  // Cancel after completion — should be a no-op, must not crash.
  handle.Cancel();
  TEST(handle.IsCancelled(), ());
}

// Multiple cancel calls should be safe.
UNIT_TEST(HttpClient_Cancel_Idempotent)
{
  HttpClient client(kTestUrlBigFile);
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;

  auto handle = client.RunHttpRequestAsync([&](HttpClient::Result)
  {
    std::lock_guard lock(mu);
    done = true;
    cv.notify_one();
  });

  handle.Cancel();
  handle.Cancel();
  handle.Cancel();

  std::unique_lock lock(mu);
  cv.wait(lock, [&] { return done; });
  TEST(handle.IsCancelled(), ());
}

}  // namespace http_client_test
