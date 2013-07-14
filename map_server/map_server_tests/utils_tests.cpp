#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../../graphics/defines.hpp"
#include "../../platform/platform.hpp"
#include "../../coding/reader.hpp"

#include "../../map_server_utils/request.hpp"
#include "../../map_server_utils/response.hpp"
#include "../../map_server_utils/viewport.hpp"

#include "../../std/iostream.hpp"

#include <QtCore/QByteArray>

UNIT_TEST(request_sending)
{
  srv::Request::Params params;
  params.m_viewport = srv::Viewport(m2::PointD(10.0, 20.0), 3, 300, 300);
  params.m_density = graphics::EDensityXXHDPI;

  srv::Request r(params);

  stringstream stream;
  srv::SendRequest(stream, r);
  srv::Request input = srv::GetRequest(stream);

  TEST_EQUAL(r.GetDensity(), input.GetDensity(), (""));
  TEST_EQUAL((int)r.GetViewport().GetCenter().x, (int)input.GetViewport().GetCenter().x, (""));
  TEST_EQUAL((int)r.GetViewport().GetCenter().y, (int)input.GetViewport().GetCenter().y, (""));
  TEST_EQUAL((int)r.GetViewport().GetScale(), (int)input.GetViewport().GetScale(), (""));
  TEST_EQUAL((int)r.GetViewport().GetWidth(), (int)input.GetViewport().GetWidth(), (""));
  TEST_EQUAL((int)r.GetViewport().GetHeight(), (int)input.GetViewport().GetHeight(), (""));
}

UNIT_TEST(get_response)
{
  srv::Request::Params params;
  params.m_viewport = srv::Viewport(m2::PointD(10.0, 20.0), 3, 300, 300);
  params.m_density = graphics::EDensityXXHDPI;

  srv::Request r(params);
  ModelReader * reader = GetPlatform().GetReader("test.png");
  uint64_t size = reader->Size();
  char * data = new char[size];
  reader->Read(0, (void *)data, size);
  delete reader;

  QByteArray bytes(data, size);

  srv::Response * response = r.CreateResponse(bytes);

  std::streambuf * backup = cout.rdbuf();
  stringstream inStream;
  cout.rdbuf(inStream.rdbuf());
  response->DoResponse();
  cout.rdbuf(backup);

  inStream.seekg (0, inStream.end);
  int length = inStream.tellg();
  inStream.seekg(0, inStream.beg);
  char * responseData = new char[length];
  inStream.read(responseData, length);

  string responseString(responseData, length);
  size_t responseSize = responseString.size();

  TEST_EQUAL(size, (uint64_t)responseSize, (""));
  TEST_EQUAL(memcmp(data, responseString.c_str(), size), 0, (""));

  delete[] data;
  delete[] responseData;
}
