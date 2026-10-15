#include "testing/testing.hpp"

#include "map/nogps/esp32_protocol.hpp"

#include <cstdio>
#include <string>

namespace nogps_esp32_protocol_tests
{
using namespace nogps;
using namespace nogps::esp32;

// Returns the line with its checksum.
std::string WithCrc(std::string const & body)
{
  char crc[8];
  std::snprintf(crc, sizeof(crc), "%04X", Crc16(body));
  return "$" + body + "*" + crc;
}

UNIT_TEST(NoGps_Esp32_Crc)
{
  // The check value of CRC-16/CCITT-FALSE.
  TEST_EQUAL(Crc16("123456789"), 0x29B1, ());
}

UNIT_TEST(NoGps_Esp32_Command)
{
  TEST_EQUAL(Command(17, "HELLO"), "$NGC,17,HELLO*D289\r\n", ());
  TEST_EQUAL(Command(18, "CAL_UP"), "$NGC,18,CAL_UP*5D27\r\n", ());
}

UNIT_TEST(NoGps_Esp32_ParsesData)
{
  auto const fields = Parse("$NGD,1,15230,845213,-1234567,-15300,5021345,43,80,1F*1BDE\r\n");
  TEST(fields, ());
  auto const data = ParseData(*fields);
  TEST(data, ());
  TEST_EQUAL(data->m_seq, 15230, ());
  TEST_EQUAL(data->m_timeMs, 845213, ());
  TEST_EQUAL(data->m_yawMdeg, -1234567, ());
  TEST_EQUAL(data->m_rateMdps, -15300, ());
  TEST_EQUAL(data->m_distMm, 5021345, ());
  TEST_EQUAL(data->m_speedKmh, 43, ());
  TEST_EQUAL(data->m_speedAgeMs, 80, ());
  TEST_EQUAL(data->m_flags, 0x1F, ());
}

UNIT_TEST(NoGps_Esp32_ParsesAccel)
{
  // The lines of the firmware with the accelerations: a speed bump.
  auto data = ParseData(*Parse("$NGD,1,5022,945253,-21882,-2100,812584,43,100,1F,-180,95,2350,6800*47CA"));
  TEST(data, ());
  TEST(data->m_hasAccel, ());
  TEST_ALMOST_EQUAL_ABS(data->m_accelH1, -0.18, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(data->m_accelH2, 0.095, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(data->m_accelUp, 2.35, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(data->m_jolt, 6.8, 1e-9, ());
  TEST_EQUAL(data->m_speedKmh, 43, ());
  // The vertical is not calibrated.
  data = ParseData(*Parse("$NGD,1,77,12345,0,0,0,-1,-1,223,,,,*9174"));
  TEST(data, ());
  TEST(!data->m_hasAccel, ());
  // An older box.
  data = ParseData(*Parse("$NGD,1,1234,845213,-15300,0,0,-1,-1,227*AEEE"));
  TEST(data, ());
  TEST(!data->m_hasAccel, ());
}

UNIT_TEST(NoGps_Esp32_RejectsBadChecksum)
{
  TEST(!Parse("$NGD,1,15230,845213,-1234567,-15300,5021345,43,80,1F*1BDF"), ());
  TEST(!Parse("NGD,1,15230*1BDE"), ());
  TEST(!Parse("$NGD,1,15230"), ());
  TEST(!Parse("$NGD,1*ZZZZ"), ());
}

UNIT_TEST(NoGps_Esp32_RejectsOtherVersion)
{
  TEST(!ParseData({"NGD", "2", "1", "1", "1", "1", "1", "1", "1", "1F"}), ());
}

UNIT_TEST(NoGps_Esp32_ParsesReplies)
{
  auto const ok = Parse("$NGA,18,OK*461D");
  TEST(ok, ());
  auto const okReply = ParseReply(*ok);
  TEST(okReply, ());
  TEST_EQUAL(okReply->m_id, 18, ());
  TEST(okReply->m_ok, ());

  auto const progress = Parse("$NGA,18,PROGRESS,40*4D95");
  TEST(progress, ());
  auto const progressReply = ParseReply(*progress);
  TEST(progressReply, ());
  TEST_EQUAL(progressReply->m_progress, 40, ());
  TEST(!progressReply->m_ok, ());

  auto const error = Parse("$NGA,18,ERR,MOVING*416A");
  TEST(error, ());
  auto const errorReply = ParseReply(*error);
  TEST(errorReply, ());
  TEST_EQUAL(errorReply->m_error, "MOVING", ());
}

UNIT_TEST(NoGps_Esp32_ParsesStatus)
{
  auto const fields = Parse("$NGS,1,1.0.0,ICM42688,400,OK,ISO 15765-4 CAN 11/500,12,412,0*8BEB");
  TEST(fields, ());
  TEST_EQUAL(ParseImuName(*fields), "ICM42688", ());
  TEST_EQUAL(ParseObdState(*fields), "OK", ());
}

UNIT_TEST(NoGps_Esp32_ParsesStatusWithoutElm327)
{
  // The fields added in the second spec follow the first ones.
  auto const fields = Parse(WithCrc("NGS,1,2.0.0,ICM42688,400,NO_ADAPTER,,12,412,0,OK,0x47,NO_ADAPTER,0,-15,3"));
  TEST(fields, ());
  TEST_EQUAL(ParseObdState(*fields), "NO_ADAPTER", ());
  TEST(!ParseObdState(*Parse("$NGA,18,OK*461D")), ());
}

UNIT_TEST(NoGps_Esp32_ParsesCarInfo)
{
  // Lines from the firmware with the engine, its speed and the voltage in the fields 17-19.
  auto car = ParseCarInfo(*Parse(
      "$NGS,1,0.2.0,ICM20602,500,OK,ISO 15765-4 (CAN 11/500),12,331,0,OK,0x12,NONE,38400,-386,9,RUN,812,14100*EA0D"));
  TEST(car, ());
  TEST_EQUAL(car->m_engineRunning, true, ());
  TEST_EQUAL(car->m_rpm, 812, ());
  TEST_EQUAL(car->m_boxMillivolts, 14100, ());
  TEST_EQUAL(car->m_elmMillivolts, -1, ());
  TEST(!car->m_voltageMismatch, ());

  car = ParseCarInfo(*Parse("$NGS,1,0.2.0,ICM20602,500,SLEEP,,12,331,0,OK,0x12,NONE,38400,-386,14,OFF,,12480*3920"));
  TEST(car, ());
  TEST_EQUAL(car->m_engineRunning, false, ());
  TEST_EQUAL(car->m_rpm, -1, ());
  TEST_EQUAL(car->m_boxMillivolts, 12480, ());

  // Nothing is known without ELM327: the last fields are empty.
  car = ParseCarInfo(*Parse("$NGS,1,0.2.0,ICM20602,500,NO_ADAPTER,,,331,0,OK,0x12,NO_ADAPTER,0,-386,4,,,*D654"));
  TEST(car, ());
  TEST(!car->m_engineRunning, ());
  TEST_EQUAL(car->m_boxMillivolts, -1, ());

  // Lines of the firmware with 21 fields: the voltage from ELM327 and from the control unit of the car.
  car =
      ParseCarInfo(*Parse("$NGS,1,0.2.0,ICM20602,500,OK,ISO 15765-4 (CAN 11/500),12,331,0,OK,0x12,NONE,38400,"
                          "-386,9,RUN,812,14100,13800,14080*AF2F"));
  TEST(car, ());
  TEST_EQUAL(car->m_boxMillivolts, 14100, ());
  TEST_EQUAL(car->m_elmMillivolts, 13800, ());
  TEST_EQUAL(car->m_ecuMillivolts, 14080, ());
  TEST_EQUAL(car->GetReferenceMillivolts(), 14080, ());
  TEST(!car->m_voltageMismatch, ());

  // The car drives: only the voltage measured by the box comes.
  car =
      ParseCarInfo(*Parse("$NGS,1,0.2.0,ICM20602,500,OK,ISO 15765-4 (CAN 11/500),3,335,0,OK,0x12,NONE,38400,-380,"
                          "9,RUN,,14230,,*3483"));
  TEST(car, ());
  TEST_EQUAL(car->m_rpm, -1, ());
  TEST_EQUAL(car->m_boxMillivolts, 14230, ());
  TEST_EQUAL(car->m_elmMillivolts, -1, ());
  TEST_EQUAL(car->m_ecuMillivolts, -1, ());

  // No divider in the box, the car doesn't tell its voltage.
  car =
      ParseCarInfo(*Parse("$NGS,1,0.2.0,ICM20602,500,OK,ISO 15765-4 (CAN 11/500),12,331,0,OK,0x12,NONE,38400,-386,"
                          "9,RUN,790,,13800,*9680"));
  TEST(car, ());
  TEST_EQUAL(car->m_boxMillivolts, -1, ());
  TEST_EQUAL(car->GetReferenceMillivolts(), 13800, ());
  TEST(!car->m_voltageMismatch, ());

  // The divider of the box is wrong if its voltage differs from the car by more than 10%.
  car =
      ParseCarInfo(*Parse(WithCrc("NGS,1,0.2.0,ICM20602,500,OK,,12,331,0,OK,0x12,NONE,38400,-386,9,RUN,812,17200,"
                                  "12400,")));
  TEST(car, ());
  TEST(car->m_voltageMismatch, ());

  // A status of an older firmware and other lines tell nothing about the car.
  TEST(!ParseCarInfo(*Parse("$NGA,18,OK*461D")), ());
}

UNIT_TEST(NoGps_Esp32_ParsesLinesOfFirmware)
{
  // Lines from the firmware 0.2.0: ELM327 is off, the box stands still.
  auto const data = ParseData(*Parse("$NGD,1,1234,845213,-15300,0,0,-1,-1,227*AEEE"));
  TEST(data, ());
  TEST_EQUAL(data->m_yawMdeg, -15300, ());
  TEST_EQUAL(data->m_speedKmh, -1, ());
  TEST_EQUAL(data->m_flags, kFlagImuOk | kFlagBiasOk | kFlagUpOk | kFlagStill | kFlagObdAbsent, ());

  auto const status = Parse("$NGS,1,0.2.0,ICM20602,500,DISABLED,,,331,0,OK,0x12,NONE,0,-386,3*9ED2");
  TEST(status, ());
  TEST_EQUAL(ParseImuName(*status), "ICM20602", ());
  TEST_EQUAL(ParseObdState(*status), "DISABLED", ());
  TEST_EQUAL(ParseLastEventNumber(*status), 3, ());

  auto const info =
      ParseInfo(*Parse("$NGI,1,0.3.3,2026-10-10T07:33,esp32s3,s3zero,C47D,4096,2031616,ota_0,VALID,WIFI BLE OTA*C597"));
  TEST(info, ());
  TEST_EQUAL(info->m_firmware, "0.3.3", ());
  TEST_EQUAL(info->m_chip, "esp32s3", ());
  TEST_EQUAL(info->m_board, "s3zero", ());
  TEST(info->m_canUpdate, ());
  TEST_EQUAL(info->m_id, "C47D", ());
  TEST(!info->m_powerOns, ());
  // A later firmware counts how many times the box was powered on.
  auto const countingInfo = ParseInfo(
      *Parse("$NGI,1,0.3.5,2026-10-10T11:00,esp32s3,s3zero,C47D,4096,2031616,ota_0,VALID,WIFI BLE OTA,12*94E1"));
  TEST(countingInfo, ());
  TEST(countingInfo->m_canUpdate, ());
  TEST(countingInfo->m_powerOns, ());
  TEST_EQUAL(*countingInfo->m_powerOns, 12, ());
  // The first firmware with Bluetooth didn't take a firmware over it.
  auto const oldInfo =
      ParseInfo(*Parse("$NGI,1,0.3.0,2026-10-08T19:43,esp32s3,s3zero,1B00,4096,2031616,ota_0,VALID,WIFI BLE*BBC4"));
  TEST(oldInfo, ());
  TEST(!oldInfo->m_canUpdate, ());
  TEST(!ParseInfo(*Parse("$NGE,1,7,845100,I,CAL_AUTO,-386*AC09")), ());

  auto const received = ParseUpdateProgress(*Parse("$NGO,1,4320,RESEND*4B18"));
  TEST(received, ());
  TEST_EQUAL(received->m_received, 4320, ());
  TEST_EQUAL(received->m_state, "RESEND", ());
  // What an accepted command returns: the size of a piece and of the window.
  auto const begun = ParseReply(*Parse("$NGA,21,OK,240,4096*88F0"));
  TEST(begun, ());
  TEST(begun->m_ok, ());
  TEST_EQUAL(begun->m_values, std::vector<std::string>({"240", "4096"}), ());

  auto const event = ParseEvent(*Parse("$NGE,1,7,845100,I,CAL_AUTO,-386*AC09"));
  TEST(event, ());
  TEST_EQUAL(event->m_number, 7, ());
  TEST_EQUAL(event->m_level, "I", ());
  TEST_EQUAL(event->m_code, "CAL_AUTO", ());
  TEST_EQUAL(event->m_text, "-386", ());
}
}  // namespace nogps_esp32_protocol_tests
