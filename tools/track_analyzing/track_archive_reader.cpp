#include "track_analyzing/track_archive_reader.hpp"

#include "track_analyzing/temporary_file.hpp"

#include "tracking/archival_file.hpp"
#include "tracking/archive.hpp"

#include "coding/file_reader.hpp"
#include "coding/hex.hpp"
#include "coding/zlib.hpp"

#include "base/logging.hpp"

#include <exception>
#include <iterator>
#include <memory>
#include <optional>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "3party/minizip/minizip.hpp"

using namespace std;
using namespace track_analyzing;

namespace track_analyzing
{
string const kTmpArchiveFileNameTemplate("-tmp-track-archive.zip");

// Track record fields:
//  0: user ID
//  1: track record version
//  2: timestamp
//  3: data size
//  4: hex coded data
constexpr size_t kTrackRecordSize = 5;
constexpr size_t kTrackRecordUserIdIndex = 0;
constexpr size_t kTrackRecordDataIndex = 4;

namespace details
{
/// \brief Take substring starting with offset until delimiter or EOL will found.
/// \returns Substring value.
string GetToken(string const & str, size_t offset, string const & delimiter);

/// \brief Parse substring starting with offset until delimiter or EOL will found.
/// \returns Substring value and set offset to position after delimiter.
string ParseString(string const & str, string const & delimiter, size_t & offset);

/// \brief Check presence of ZIP file magick bytes header in file content.
bool HasZipSignature(string const & binaryData);

struct UserTrackInfo
{
  string m_userId;
  Track m_track;
};

/// \brief Parse log records with tracks produced by track_archiver.
/// \param record The log line.
/// \param tmpArchiveFile The temporary file for zip archive unpacking
///        (reused across multiple invocations).
/// \returns User ID and vector of track points if parsed successfully.
optional<UserTrackInfo> ParseLogRecord(string const & record, TemporaryFile & tmpArchiveFile);

/// \brief Parse ZIP archive from multipart/form-data.
/// \param binaryData HTTP body bytes.
/// \returns Decoded file content if present and succesfully parsed.
optional<string> ParseMultipartData(string const & binaryData);

/// \brief Parse track file from ZIP archive stream.
/// \param zipReader The ZIP stream.
/// \param trackData The track points data (out param).
/// \returns Returns true if file parsed succesfully and fill trackData.
bool ParseTrackFile(unzip::File & zipReader, Track & trackData) noexcept;

/// \brief Unpack track points (delta coded and compressed) from memory buffer.
/// \param data The buffer pointer.
/// \param dataSize The buffer size.
/// \param trackData The track points data (out param).
/// \returns Returns true if file parsed succesfully and fill trackData.
template <typename Reader, typename Pack>
bool ReadTrackFromArchive(char const * data, size_t dataSize, Track & trackData) noexcept;

string GetToken(string const & str, size_t offset, string const & delimiter)
{
  size_t endPos = str.find(delimiter, offset);
  if (endPos == string::npos)
    return str.substr(offset, str.size() - offset);
  return str.substr(offset, endPos - offset);
}

string ParseString(string const & str, string const & delimiter, size_t & offset)
{
  string result = GetToken(str, offset, delimiter);
  offset += result.size() + delimiter.size();
  if (offset > str.size())  // missing delimiter at string end
    offset = str.size();
  return result;
}

optional<string> ParseMultipartData(string const & binaryData)
{
  // Fast check multipart/form-data format (content starts with boundary string
  // and boundary string must starts with two dashes --)
  if (binaryData.size() < 2 || binaryData[0] != '-' || binaryData[1] != '-')
    return nullopt;

  // Parse multipart headers
  string lineDelimiter("\r\n");
  size_t offset = 0;

  bool hasContentTypeHeader = false;
  bool hasContentDispositionHeader = false;

  string expectedContentType("Content-Type: application/zip");
  string expectedContentDispositionPrefix("Content-Disposition: form-data; name=\"file\";");

  string boundary = ParseString(binaryData, lineDelimiter, offset);
  for (string header = ParseString(binaryData, lineDelimiter, offset); !header.empty();
       header = ParseString(binaryData, lineDelimiter, offset))
  {
    if (expectedContentType == header)
      hasContentTypeHeader = true;
    if (expectedContentDispositionPrefix.compare(0, string::npos, header, 0, expectedContentDispositionPrefix.size()) ==
        0)
    {
      hasContentDispositionHeader = true;
    }
  }

  if (!hasContentTypeHeader && !hasContentDispositionHeader)
    return nullopt;

  // Parse file content until boundary
  return ParseString(binaryData, boundary, offset);
}

bool HasZipSignature(string const & binaryData)
{
  return binaryData.size() >= 4 && binaryData[0] == 0x50 && binaryData[1] == 0x4b && binaryData[2] == 0x03 &&
         binaryData[3] == 0x04;
}

template <typename Reader, typename Pack>
bool ReadTrackFromArchive(char const * data, size_t dataSize, Track & trackData) noexcept
{
  try
  {
    coding::ZLib::Inflate inflate(coding::ZLib::Inflate::Format::ZLib);
    vector<uint8_t> buffer;
    inflate(data, dataSize, back_inserter(buffer));

    ReaderSource<MemReaderWithExceptions> reader(MemReaderWithExceptions(buffer.data(), buffer.size()));
    if (reader.Size() == 0)
      return false;

    // Read first point.
    Pack point = tracking::TraitsPacket<Pack>::Read(reader, false /* isDelta */);
    trackData.emplace_back(point.m_timestamp, ms::LatLon(point.m_lat, point.m_lon),
                           static_cast<uint8_t>(tracking::TraitsPacket<Pack>::GetSpeedGroup(point)));

    // Read with delta.
    while (reader.Size() > 0)
    {
      Pack const delta = tracking::TraitsPacket<Pack>::Read(reader, true /* isDelta */);
      point = tracking::TraitsPacket<Pack>::Combine(point, delta);

      trackData.emplace_back(point.m_timestamp, ms::LatLon(point.m_lat, point.m_lon),
                             static_cast<uint8_t>(tracking::TraitsPacket<Pack>::GetSpeedGroup(point)));
    }
    return true;
  }
  catch (exception const & e)
  {
    LOG(LWARNING, ("Error reading track file:", e.what()));
  }
  return false;
}

bool ParseTrackFile(unzip::File & zipReader, Track & trackData) noexcept
{
  unzip::FileInfo fileInfo;
  if (unzip::GetCurrentFileInfo(zipReader, fileInfo) != unzip::Code::Ok)
  {
    LOG(LERROR, ("Unable to get file info from zip archive"));
    return false;
  }

  auto archiveInfo = tracking::archival_file::ParseArchiveFilename(fileInfo.m_filename);

  if (unzip::OpenCurrentFile(zipReader) != unzip::Code::Ok)
  {
    LOG(LERROR, ("Unable to open file from zip archive"));
    return false;
  }

  unzip::Buffer fileData;
  int dataSize = unzip::ReadCurrentFile(zipReader, fileData);
  if (dataSize < 0)
  {
    LOG(LERROR, ("Unable to read file from zip archive"));
    return false;
  }

  bool result = false;
  if (archiveInfo.m_trackType == routing::RouterType::Vehicle)
  {
    result = ReadTrackFromArchive<ReaderSource<MemReaderWithExceptions>, tracking::PacketCar>(fileData.data(), dataSize,
                                                                                              trackData);
  }
  else
  {
    result = ReadTrackFromArchive<ReaderSource<MemReaderWithExceptions>, tracking::Packet>(fileData.data(), dataSize,
                                                                                           trackData);
  }

  if (unzip::CloseCurrentFile(zipReader) != unzip::Code::Ok)
  {
    LOG(LERROR, ("Unable to close file from zip archive"));
    return false;
  }

  return result;
}

optional<Track> ParseTrackArchiveData(string const & content, TemporaryFile & tmpArchiveFile)
{
  string binaryData = FromHex(content);
  optional<string> archiveBody;

  if (HasZipSignature(binaryData))
  {
    archiveBody = binaryData;
  }
  else
  {
    archiveBody = ParseMultipartData(binaryData);
    if (!archiveBody)
    {
      LOG(LERROR, ("Bad HTTP body (expect multipart/form-data or application/zip):", content));
      return nullopt;
    }
  }

  tmpArchiveFile.WriteData(*archiveBody);

  unzip::File zipReader = unzip::Open(tmpArchiveFile.GetFilePath().c_str());

  Track trackData;

  bool result = ParseTrackFile(zipReader, trackData);
  while (result && unzip::GoToNextFile(zipReader) == unzip::Code::Ok)
    result = ParseTrackFile(zipReader, trackData);

  if (unzip::Close(zipReader) != unzip::Code::Ok)
    LOG(LERROR, ("Unable to close temporary zip archive"));

  return result ? optional<Track>(std::move(trackData)) : nullopt;
}

optional<UserTrackInfo> ParseLogRecord(string const & record, TemporaryFile & tmpArchiveFile)
{
  vector<string> items;
  boost::split(items, record, boost::is_any_of("\t"));

  if (items.size() != kTrackRecordSize)
  {
    LOG(LERROR, ("Bad log record: ", record));
    return nullopt;
  }

  optional<Track> track = ParseTrackArchiveData(items[kTrackRecordDataIndex], tmpArchiveFile);

  if (track)
    return optional<UserTrackInfo>({items[kTrackRecordUserIdIndex], std::move(*track)});

  return nullopt;
}

}  // namespace details

void TrackArchiveReader::ParseUserTracksFromFile(string const & logFile, UserToTrack & userToTrack) const
{
  // Read file content
  FileReader reader(logFile);
  string archiveData;
  reader.ReadAsString(archiveData);

  // Unzip data
  using Inflate = coding::ZLib::Inflate;
  Inflate inflate(Inflate::Format::GZip);
  string logData;
  inflate(archiveData.data(), archiveData.size(), back_inserter(logData));

  // Parse log
  TemporaryFile tmpArchiveFile("" /* empty prefix */, kTmpArchiveFileNameTemplate);
  stringstream logParser;
  logParser << logData;

  size_t linesCount = 0;
  size_t errorsCount = 0;
  for (string line; getline(logParser, line); ++linesCount)
  {
    optional<details::UserTrackInfo> data = details::ParseLogRecord(line, tmpArchiveFile);

    if (data)
    {
      Track & track = userToTrack[data->m_userId];
      track.insert(track.end(), data->m_track.cbegin(), data->m_track.cend());
    }
    else
    {
      ++errorsCount;
    }
  }

  LOG(LINFO, ("Process", linesCount, "log records"));
  if (errorsCount == 0)
    LOG(LINFO, ("All records are parsed successfully"));
  else
    LOG(LERROR, ("Unable to parse", errorsCount, "records"));
}
}  // namespace track_analyzing
