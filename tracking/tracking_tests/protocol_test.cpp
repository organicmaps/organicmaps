#include "testing/testing.hpp"

#include "tracking/protocol.hpp"

using namespace tracking;

UNIT_TEST(Protocol_CreateAuthPacket)
{
  auto packet = Protocol::CreateAuthPacket("AAA");
  TEST_EQUAL(packet.size(), 7, ());
  TEST_EQUAL(Protocol::PacketType(packet[0]), Protocol::PacketType::CurrentAuth, ());
  TEST_EQUAL(packet[1], 0x00, ());
  TEST_EQUAL(packet[2], 0x00, ());
  TEST_EQUAL(packet[3], 0x03, ());
  TEST_EQUAL(packet[4], 'A', ());
  TEST_EQUAL(packet[5], 'A', ());
  TEST_EQUAL(packet[6], 'A', ());
}

UNIT_TEST(Protocol_CreateDataPacket)
{
  Protocol::DataElements buffer(5);
  buffer.push_back(Protocol::DataElements::value_type(1, ms::LatLon(10, 10)));
  buffer.push_back(Protocol::DataElements::value_type(2, ms::LatLon(15, 15)));
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
