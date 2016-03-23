#include "testing/testing.hpp"

#include "coding/reader_wrapper.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "platform/mwm_version.hpp"

#include "std/string.hpp"

namespace
{
string WriteMwmVersion(version::Format const format, uint64_t const version)
{
  string data;
  MemWriter<string> writer(data);
  WriterSink<MemWriter<string>> sink(writer);

  char const prolog[] = "MWM";
  sink.Write(prolog, ARRAY_SIZE(prolog));

  WriteVarUint(sink, static_cast<uint32_t>(format));
  WriteVarUint(sink, version);

  return data;
}

version::MwmVersion ReadMwmVersion(string const & data)
{
  MemReader reader(data.data(), data.size());
  ReaderSrc source(reader);

  version::MwmVersion version;
  version::ReadVersion(source, version);

  return version;
}
}  // namespace

UNIT_TEST(MwmVersion_OldFormat)
{
  uint64_t const secondsSinceEpoch = 1455235200;  // 160212
  uint32_t const dataVersion = 160212;
  // Before Format::v8 there was a data version written to mwm.
  string const data = WriteMwmVersion(version::Format::v7, dataVersion);
  version::MwmVersion const mwmVersion = ReadMwmVersion(data);
  TEST_EQUAL(mwmVersion.GetSecondsSinceEpoch(), secondsSinceEpoch, ());
  TEST_EQUAL(mwmVersion.GetFormat(), version::Format::v7, ());
  TEST_EQUAL(mwmVersion.GetVersion(), dataVersion, ());
}

UNIT_TEST(MwmVersion_NewFormat)
{
  uint64_t const secondsSinceEpoch = 1455870947;  // 160219
  uint32_t const dataVersion = 160219;
  // After Format::v8 seconds since epoch are stored in mwm.
  string const data = WriteMwmVersion(version::Format::v8, secondsSinceEpoch);
  version::MwmVersion const mwmVersion = ReadMwmVersion(data);
  TEST_EQUAL(mwmVersion.GetSecondsSinceEpoch(), secondsSinceEpoch, ());
  TEST_EQUAL(mwmVersion.GetFormat(), version::Format::v8, ());
  TEST_EQUAL(mwmVersion.GetVersion(), dataVersion, ());
}
