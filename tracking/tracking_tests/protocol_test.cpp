#include "testing/testing.hpp"

#include "tracking/protocol.hpp"

using namespace tracking;

UNIT_TEST(Protocol_CreateAuthPacket)
{
  auto packet = Protocol::CreateAuthPacket("ABC");
  TEST_EQUAL(packet.size(), 7, ());
  TEST_EQUAL(Protocol::PacketType(packet[0]), Protocol::PacketType::CurrentAuth, ());
  TEST_EQUAL(packet[1], 0x00, ());
  TEST_EQUAL(packet[2], 0x00, ());
  TEST_EQUAL(packet[3], 0x03, ());
  TEST_EQUAL(packet[4], 'A', ());
  TEST_EQUAL(packet[5], 'B', ());
  TEST_EQUAL(packet[6], 'C', ());
}

UNIT_TEST(Protocol_DecodeHeader)
{
  string id_str("ABC");
  auto packet = Protocol::CreateAuthPacket(id_str);
  TEST_EQUAL(packet.size(), 7, ());
  TEST_EQUAL(Protocol::PacketType(packet[0]), Protocol::PacketType::CurrentAuth, ());

  auto header = Protocol::DecodeHeader(packet);
  CHECK_EQUAL(header.first, Protocol::PacketType::CurrentAuth, ());
  CHECK_EQUAL(header.second, id_str.size(), ());
}

UNIT_TEST(Protocol_CreateDataPacket)
{
  using Container = Protocol::DataElementsCirc;
  Container buffer(5);
  buffer.push_back(Container::value_type(1, ms::LatLon(10, 10)));
  buffer.push_back(Container::value_type(2, ms::LatLon(15, 15)));
  auto packet = Protocol::CreateDataPacket(buffer);
  TEST_EQUAL(packet.size(), 26, ());
  TEST_EQUAL(Protocol::PacketType(packet[0]), Protocol::PacketType::CurrentData, ());
  TEST_EQUAL(packet[1], 0x00, ());
  TEST_EQUAL(packet[2], 0x00, ());
  TEST_EQUAL(packet[3], 22, ());
  TEST_EQUAL(packet[4], 1, ());
  TEST_EQUAL(packet[5], 227, ());
  TEST_EQUAL(packet[6], 241, ());
}

UNIT_TEST(Protocol_DecodeAuthPacket)
{
  auto packet = Protocol::CreateAuthPacket("ABC");
  TEST_EQUAL(packet.size(), 7, ());
  TEST_EQUAL(Protocol::PacketType(packet[0]), Protocol::PacketType::CurrentAuth, ());

  auto result = Protocol::DecodeAuthPacket(Protocol::PacketType::CurrentAuth, packet);
  TEST_EQUAL(result, "ABC", ());
}

UNIT_TEST(Protocol_DecodeDataPacket)
{
  double const kEps = 1e-5;

  using Container = Protocol::DataElementsVec;

  Container points;
  points.push_back(Container::value_type(1, ms::LatLon(10, 10)));
  points.push_back(Container::value_type(2, ms::LatLon(15, 15)));
  auto packet = Protocol::CreateDataPacket(points);
  TEST_EQUAL(packet.size(), 26, ());
  TEST_EQUAL(Protocol::PacketType(packet[0]), Protocol::PacketType::CurrentData, ());

  Container result = Protocol::DecodeDataPacket(Protocol::PacketType::CurrentData, packet);

  TEST_EQUAL(points.size(), result.size(), ());
  for (size_t i = 0; i < points.size(); ++i)
  {
    TEST_EQUAL(points[i].m_timestamp, result[i].m_timestamp,
               (points[i].m_timestamp, result[i].m_timestamp));
    TEST(my::AlmostEqualAbsOrRel(points[i].m_latLon.lat, result[i].m_latLon.lat, kEps),
         (points[i].m_latLon.lat, result[i].m_latLon.lat));
    TEST(my::AlmostEqualAbsOrRel(points[i].m_latLon.lon, result[i].m_latLon.lon, kEps),
         (points[i].m_latLon.lon, result[i].m_latLon.lon));
  }
}
