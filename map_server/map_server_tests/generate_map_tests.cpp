#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../../coding/reader.hpp"
#include "../../coding/base64.hpp"

#include "../../platform/platform.hpp"

#include "../../map_server_utils/request.hpp"
#include "../../map_server_utils/viewport.hpp"

#include <QtCore/QProcess>
#include <QtCore/QByteArray>

UNIT_TEST(generate_simle_map)
{
  QProcess * mapServer = new QProcess();
  mapServer->start("./map_server", QIODevice::ReadWrite);
  if (!mapServer->waitForStarted())
    return;

  ::srv::Request::Params params;
  params.m_density = graphics::EDensityMDPI;
  params.m_viewport = ::srv::Viewport(m2::PointD(0.0, 0.0), 1, 128, 128);
  ::srv::Request r(params);

  stringstream ss;
  ::srv::SendRequest(ss, r);
  string sendData = ss.str();
  mapServer->write(sendData.c_str(), sendData.size());
  mapServer->waitForFinished();

  QByteArray response = mapServer->readAll();
  string responseString(response.constData(), response.size());
  responseString = base64::Decode(responseString);

  ModelReader * reader = GetPlatform().GetReader("test.png");
  uint64_t size = reader->Size();
  char * data = new char[size];
  reader->Read(0, (void *)data, size);
  delete reader;

  TEST_EQUAL(size, responseString.size(), (""));
  TEST_EQUAL(memcmp(data, responseString.c_str(), size), 0, (""));
}
