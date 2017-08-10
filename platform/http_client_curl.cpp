/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/
#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/zlib.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp"

#include "std/array.hpp"
#include "std/fstream.hpp"
#include "std/iterator.hpp"
#include "std/sstream.hpp"
#include "std/vector.hpp"

#include <cstdio>    // popen, tmpnam

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>  // close
#endif

using namespace coding;

namespace
{
DECLARE_EXCEPTION(PipeCallError, RootException);

struct ScopedRemoveFile
{
  ScopedRemoveFile() = default;
  explicit ScopedRemoveFile(string const & fileName) : m_fileName(fileName) {}

  ~ScopedRemoveFile()
  {
    if (!m_fileName.empty())
      std::remove(m_fileName.c_str());
  }

  std::string m_fileName;
};

static string ReadFileAsString(string const & filePath)
{
  ifstream ifs(filePath, ifstream::in);
  if (!ifs.is_open())
    return {};

  return {istreambuf_iterator<char>(ifs), istreambuf_iterator<char>()};
}


string RunCurl(string const & cmd)
{
  FILE * pipe = ::popen(cmd.c_str(), "r");
  ASSERT(pipe, ());
  array<char, 8 * 1024> arr;
  string result;
  size_t read;
  do
  {
    read = ::fread(arr.data(), 1, arr.size(), pipe);
    if (read > 0)
    {
      result.append(arr.data(), read);
    }
  } while (read == arr.size());

  auto const err = ::pclose(pipe);
  // Exception will be cought in RunHTTPRequest
  if (err)
    throw PipeCallError("", "Error " + strings::to_string(err) + " while calling " + cmd);

  return result;
}

string GetTmpFileName()
{  
  boost::uuids::random_generator gen;
  boost::uuids::uuid u = gen();

  stringstream ss;
  ss << u;

  ASSERT(!ss.str().empty(), ());

  return GetPlatform().TmpPathForFile(ss.str());
}

typedef vector<pair<string, string>> Headers;

Headers ParseHeaders(string const & raw)
{
  istringstream stream(raw);
  Headers headers;
  string line;
  while (getline(stream, line))
  {
    auto const cr = line.rfind('\r');
    if (cr != string::npos)
      line.erase(cr);

    auto const delims = line.find(": ");
    if (delims != string::npos)
      headers.emplace_back(line.substr(0, delims), line.substr(delims + 2));
  }
  return headers;
}

bool WriteToFile(string const & fileName, string const & data)
{
  ofstream ofs(fileName);
  if(!ofs.is_open())
  {
    LOG(LERROR, ("Failed to write into a temporary file."));
    return false;
  }

  ofs << data;
  return true;
}

std::string Decompress(std::string const & compressed, std::string const & encoding)
{
  std::string decompressed;

  if (encoding == "deflate")
  {
    ZLib::Inflate inflate(ZLib::Inflate::Format::ZLib);
    inflate(compressed, back_inserter(decompressed));
  }
  else
  {
    ASSERT(false, ("Unsupported Content-Encoding:", encoding));
  }

  return decompressed;
}
}  // namespace
// Used as a test stub for basic HTTP client implementation.
// Make sure that you have curl installed in the PATH.
// TODO(AlexZ): Not a production-ready implementation.
namespace platform
{
// Extract HTTP headers via temporary file with -D switch.
// HTTP status code is extracted from curl output (-w switches).
// Redirects are handled recursively. TODO(AlexZ): avoid infinite redirects loop.
bool HttpClient::RunHttpRequest()
{
  ScopedRemoveFile headers_deleter(GetTmpFileName());
  ScopedRemoveFile body_deleter;
  ScopedRemoveFile received_file_deleter;

  string cmd = "curl -s -w '%{http_code}' -X " + m_httpMethod + " -D '" + headers_deleter.m_fileName + "' ";

  for (auto const & header : m_headers)
  {
    cmd += "-H '" + header.first + ": " + header.second + "' ";
  }

  if (!m_cookies.empty())
    cmd += "-b '" + m_cookies + "' ";

  cmd += "-m '" + strings::to_string(m_timeoutSec) + "' ";

  if (!m_bodyData.empty())
  {
    body_deleter.m_fileName = GetTmpFileName();
    // POST body through tmp file to avoid breaking command line.
    if (!WriteToFile(body_deleter.m_fileName, m_bodyData))
      return false;

    // TODO(AlexZ): Correctly clean up this internal var to avoid client confusion.
    m_inputFile = body_deleter.m_fileName;
  }
  // Content-Length is added automatically by curl.
  if (!m_inputFile.empty())
    cmd += "--data-binary '@" + m_inputFile + "' ";

  // Use temporary file to receive data from server.
  // If user has specified file name to save data, it is not temporary and is not deleted automatically.
  string rfile = m_outputFile;
  if (rfile.empty())
  {
    rfile = GetTmpFileName();
    received_file_deleter.m_fileName = rfile;
  }

  cmd += "-o " + rfile + strings::to_string(" ") + "'" + m_urlRequested + "'";

  LOG(LDEBUG, ("Executing", cmd));

  try
  {
    m_errorCode = stoi(RunCurl(cmd));
  }
  catch (RootException const & ex)
  {
    LOG(LERROR, (ex.Msg()));
    return false;
  }

  m_headers.clear();
  Headers const headers = ParseHeaders(ReadFileAsString(headers_deleter.m_fileName));
  string serverCookies;
  string headerKey;
  for (auto const & header : headers)
  {
    if (header.first == "Set-Cookie")
    {
      serverCookies += header.second + ", ";
    }
    else
    {
      if (header.first == "Location")
        m_urlReceived = header.second;

      if (m_loadHeaders)
      {
        headerKey = header.first;
        strings::AsciiToLower(headerKey);
        m_headers.emplace(headerKey, header.second);
      }
    }
  }
  m_headers.emplace("Set-Cookie", NormalizeServerCookies(move(serverCookies)));

  if (m_urlReceived.empty())
  {
    m_urlReceived = m_urlRequested;
    // Load body contents in final request only (skip redirects).
    // Sometimes server can reply with empty body, and it's ok.
    if (m_outputFile.empty())
      m_serverResponse = ReadFileAsString(rfile);
  }
  else
  {
    // Handle HTTP redirect.
    // TODO(AlexZ): Should we check HTTP redirect code here?
    LOG(LDEBUG, ("HTTP redirect", m_errorCode, "to", m_urlReceived));

    HttpClient redirect(m_urlReceived);
    redirect.SetCookies(CombinedCookies());

    if (!redirect.RunHttpRequest())
    {
      m_errorCode = -1;
      return false;
    }

    m_errorCode = redirect.ErrorCode();
    m_urlReceived = redirect.UrlReceived();
    m_headers = move(redirect.m_headers);
    m_serverResponse = move(redirect.m_serverResponse);
  }

  auto const it = m_headers.find("content-encoding");
  if (it != m_headers.end())
  {
    m_serverResponse = Decompress(m_serverResponse, it->second);
    LOG(LDEBUG, ("Response with", it->second, "is decompressed."));
  }

  return true;
}
}  // namespace platform
