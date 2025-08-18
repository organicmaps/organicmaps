#include "testing/testing.hpp"

#include "track_analyzing/temporary_file.hpp"
#include "track_analyzing/track_archive_reader.hpp"

#include "tracking/archival_file.hpp"
#include "tracking/archival_reporter.hpp"
#include "tracking/archive.hpp"

#include "coding/hex.hpp"
#include "coding/zip_creator.hpp"

#include <optional>
#include <sstream>
#include <string>

#include "3party/minizip/minizip.hpp"

namespace track_analyzing
{
namespace details
{
std::string GetToken(std::string const & str, size_t offset, std::string const & delimiter);
std::string ParseString(std::string const & str, std::string const & delimiter, size_t & offset);

bool HasZipSignature(std::string const & binaryData);

struct UserTrackInfo
{
  std::string m_userId;
  Track m_track;
};

std::optional<UserTrackInfo> ParseLogRecord(std::string const & record, TemporaryFile & tmpArchiveFile);
std::optional<std::string> ParseMultipartData(std::string const & binaryData);
bool ParseTrackFile(unzip::File & zipReader, Track & trackData) noexcept;
template <typename Reader, typename Pack>
bool ReadTrackFromArchive(char const * data, size_t dataSize, Track & trackData) noexcept;
}  // namespace details
}  // namespace track_analyzing

namespace
{
using namespace std;
using namespace track_analyzing;
using namespace track_analyzing::details;

constexpr double kAccuracyEps = 1e-4;

UNIT_TEST(UnpackTrackArchiveDataTest)
{
  // Step 1: Test data
  Track testTrack;
  testTrack.emplace_back(1577826000, ms::LatLon(55.270, 37.400), 0 /* G0 speed group */);
  testTrack.emplace_back(1577826001, ms::LatLon(55.270, 37.401), 0 /* G0 speed group */);
  testTrack.emplace_back(1577826002, ms::LatLon(55.271, 37.402), 0 /* G0 speed group */);
  testTrack.emplace_back(1577826003, ms::LatLon(55.272, 37.403), 0 /* G0 speed group */);
  testTrack.emplace_back(1577826005, ms::LatLon(55.273, 37.404), 0 /* G0 speed group */);

  string const testUserId("0PpaB8NpazZYafAxUAphkuMY51w=");

  // Step 2: Generate archive
  string const archiveFileName = tracking::archival_file::GetArchiveFilename(
      1 /* protocolVersion */, std::chrono::seconds(1577826000), routing::RouterType::Vehicle);

  // Step 2.1: Fill archive with data points
  tracking::ArchiveCar archive(tracking::kItemsForDump, tracking::kMinDelaySecondsCar);
  for (auto const & point : testTrack)
  {
    archive.Add(point.m_latLon.m_lat, point.m_latLon.m_lon, uint32_t(point.m_timestamp),
                traffic::SpeedGroup(point.m_traffic));
  }

  // Step 2.2: Store track file
  {
    FileWriter archiveWriter(archiveFileName);
    TEST_EQUAL(archive.Write(archiveWriter), true, ("Unable to write track file"));
    archiveWriter.Flush();
  }

  // Step 2.2: Archive track files batch
  vector<string> trackFiles;
  trackFiles.push_back(archiveFileName);
  string const containerFileName("test_track_archive.zip");
  TEST_EQUAL(CreateZipFromFiles(trackFiles, containerFileName, CompressionLevel::NoCompression), true,
             ("Unable to create tracks archive"));
  FileWriter::DeleteFileX(archiveFileName);

  // Step 2.3: Read batch archive content
  vector<char> buffer;
  {
    FileReader containerReader(containerFileName);
    buffer.resize(containerReader.Size());
    containerReader.Read(0 /* file begin */, buffer.data(), buffer.size());
  }
  FileWriter::DeleteFileX(containerFileName);

  // Step 2.4: Wrap as multipart data
  stringstream multipartStream;
  multipartStream << "------0000000000000\r\n";
  multipartStream << "Content-Disposition: form-data; name=\"file\"; filename=\"" << containerFileName << "\"\r\n";
  multipartStream << "Content-Type: application/zip\r\n";
  multipartStream << "\r\n";
  multipartStream.write(buffer.data(), buffer.size());
  multipartStream << "\r\n";
  multipartStream << "------0000000000000--\r\n";

  string multipartData = multipartStream.str();

  stringstream logStream;
  logStream << testUserId << "\t1\t1577826010\t" << multipartData.size() << "\t" << ToHex(multipartData);

  string const logRecord = logStream.str();

  // Unpack log record
  TemporaryFile tmpArchiveFile("tmp-unittest", ".zip");

  optional<track_analyzing::details::UserTrackInfo> data = ParseLogRecord(logRecord, tmpArchiveFile);
  TEST_EQUAL(bool(data), true, ("Unable parse track archive record"));

  TEST_EQUAL(data->m_userId, testUserId, ());

  TEST_EQUAL(data->m_track.size(), testTrack.size(), ());
  for (size_t i = 0; i < testTrack.size(); ++i)
  {
    TEST_EQUAL(data->m_track[i].m_timestamp, testTrack[i].m_timestamp, ());
    TEST_ALMOST_EQUAL_ABS(data->m_track[i].m_latLon.m_lat, testTrack[i].m_latLon.m_lat, kAccuracyEps, ());
    TEST_ALMOST_EQUAL_ABS(data->m_track[i].m_latLon.m_lon, testTrack[i].m_latLon.m_lon, kAccuracyEps, ());
    TEST_EQUAL(data->m_track[i].m_traffic, testTrack[i].m_traffic, ());
  }
}

UNIT_TEST(ParseMultipartDataTest)
{
  string const multipartData(
      "------1599512558929\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"1_1599034479_2.track.zip\"\r\n"
      "Content-Type: application/zip\r\n"
      "\r\n"
      "PK\x03\x04\x14\x00\x00\x00\b\x00\x00\x00 \x00\x8D\xF4\x84~2\x00\x00\x00-"
      "\x00\x00\x00B\x00\x00\x00/storage/emulated/0/MapsWithMe/tracks_archive/1"
      "_1599034479_2.track\x01-\x00\xD2\xFFx\xDA;\xF6\xB6q\r\xCF\xAE\xFD;z9v,"
      "\xDA\xFB\x8B\xB5\xE3\xCA\xBF\xFF\xEC\xDF\xDF\x35\x34pLel\x00\x02\x0E0"
      "\xC1\x34\x84\x99\x00\xC8\xEAX\xF0PK\x01\x02\x00\x00\x14\x00\x00\x00\b"
      "\x00\x00\x00 \x00\x8D\xF4\x84~2\x00\x00\x00-\x00\x00\x00B\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00/storage/emulate"
      "d/0/MapsWithMe/tracks_archive/1_1599034479_2.trackPK\x05\x06\x00\x00\x00"
      "\x00\x01\x00\x01\x00p\x00\x00\x00\x92\x00\x00\x00\x00\x00\r\n"
      "------1599512558929--\r\n");
  string const expectedContent(
      "PK\x03\x04\x14\x00\x00\x00\b\x00\x00\x00 \x00\x8D\xF4\x84~2\x00\x00\x00-"
      "\x00\x00\x00B\x00\x00\x00/storage/emulated/0/MapsWithMe/tracks_archive/1"
      "_1599034479_2.track\x01-\x00\xD2\xFFx\xDA;\xF6\xB6q\r\xCF\xAE\xFD;z9v,"
      "\xDA\xFB\x8B\xB5\xE3\xCA\xBF\xFF\xEC\xDF\xDF\x35\x34pLel\x00\x02\x0E0"
      "\xC1\x34\x84\x99\x00\xC8\xEAX\xF0PK\x01\x02\x00\x00\x14\x00\x00\x00\b"
      "\x00\x00\x00 \x00\x8D\xF4\x84~2\x00\x00\x00-\x00\x00\x00B\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00/storage/emulate"
      "d/0/MapsWithMe/tracks_archive/1_1599034479_2.trackPK\x05\x06\x00\x00\x00"
      "\x00\x01\x00\x01\x00p\x00\x00\x00\x92\x00\x00\x00\x00\x00\r\n");

  optional<string> content = ParseMultipartData(multipartData);
  TEST_EQUAL(bool(content), true, ());
  TEST_EQUAL(*content, expectedContent, ());
}

UNIT_TEST(ParseStringTest)
{
  string const str("--token\r\nLine1\r\nMulti\nline\r\nPrefix\rline\r\n\r\n--token--\r\n");
  string const delimiter("\r\n");
  size_t offset = 0;

  TEST_EQUAL(ParseString(str, delimiter, offset), "--token", ());
  TEST_EQUAL(ParseString(str, delimiter, offset), "Line1", ());
  TEST_EQUAL(ParseString(str, delimiter, offset), "Multi\nline", ());
  TEST_EQUAL(ParseString(str, delimiter, offset), "Prefix\rline", ());
  TEST_EQUAL(ParseString(str, delimiter, offset), "", ());
  TEST_EQUAL(ParseString(str, delimiter, offset), "--token--", ());
  TEST_EQUAL(offset, str.size(), ());

  TEST_EQUAL(ParseString(str, delimiter, offset), "", ());
  TEST_EQUAL(offset, str.size(), ());

  TEST_EQUAL(ParseString(str, delimiter, offset), "", ());
  TEST_EQUAL(offset, str.size(), ());
}

UNIT_TEST(ParseFullStringTest)
{
  string const str("no delimiter");
  string const delimiter("\r\n");
  size_t offset = 0;

  TEST_EQUAL(ParseString(str, delimiter, offset), str, ());
  TEST_EQUAL(offset, str.size(), ());
}

UNIT_TEST(CheckZipSignatureTest)
{
  string const zipLikeContent("\x50\x4b\x03\x04\x00\x00\x00\x00");
  TEST_EQUAL(HasZipSignature(zipLikeContent), true, ());

  string const nonzipContent("------1599512558929\r\n------1599512558929--\r\n");
  TEST_EQUAL(HasZipSignature(nonzipContent), false, ());

  string const shortString("yes");
  TEST_EQUAL(HasZipSignature(nonzipContent), false, ());
}

}  // namespace
