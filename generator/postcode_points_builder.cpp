#include "generator/postcode_points_builder.hpp"

#include "search/postcode_points.hpp"
#include "search/search_index_values.hpp"

#include "indexer/search_string_utils.hpp"
#include "indexer/trie_builder.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage_defines.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"
#include "coding/reader_writer_ops.hpp"
#include "coding/writer.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <fstream>
#include <vector>

#include "defines.hpp"

namespace indexer
{
namespace
{
struct FieldIndices
{
  size_t m_postcodeIndex = 0;
  size_t m_latIndex = 0;
  size_t m_longIndex = 0;
  size_t m_datasetCount = 0;
};

using ZipIndexValue = std::pair<strings::UniString, Uint64IndexValue>;

void GetPostcodes(std::string const & filename, storage::CountryId const & countryId, FieldIndices const & fieldIndices,
                  char const * separator, bool hasHeader, storage::CountryInfoGetter const & infoGetter,
                  std::vector<m2::PointD> & valueMapping, std::vector<ZipIndexValue> & keyValuePairs)
{
  ASSERT(!filename.empty(), ());
  std::ifstream data(filename);
  if (!data.is_open())
  {
    LOG(LERROR, ("Can't open postcodes file:", filename));
    return;
  }

  std::string line;

  if (hasHeader && !std::getline(data, line))
    return;

  while (std::getline(data, line))
  {
    std::vector<std::string_view> fields;
    strings::Tokenize(std::string_view(line), separator, [&fields](std::string_view v)
    {
      strings::Trim(v, "\" ");
      // Maybe empty, checks below should work for actual values.
      fields.push_back(v);
    });
    CHECK_GREATER_OR_EQUAL(fields.size(), fieldIndices.m_datasetCount, (line));

    double lat;
    CHECK(strings::to_double(fields[fieldIndices.m_latIndex], lat), (line));

    double lon;
    CHECK(strings::to_double(fields[fieldIndices.m_longIndex], lon), (line));

    auto const p = mercator::FromLatLon(lat, lon);

    std::vector<storage::CountryId> countries;
    infoGetter.GetRegionsCountryId(p, countries, 200.0 /* lookupRadiusM */);
    if (std::find(countries.begin(), countries.end(), countryId) == countries.end())
      continue;

    keyValuePairs.emplace_back(search::NormalizeAndSimplifyString(fields[fieldIndices.m_postcodeIndex]),
                               valueMapping.size());
    valueMapping.push_back(p);
  }
}

void GetUKPostcodes(std::string const & filename, storage::CountryId const & countryId,
                    storage::CountryInfoGetter const & infoGetter, std::vector<m2::PointD> & valueMapping,
                    std::vector<ZipIndexValue> & keyValuePairs)
{
  // * Follow these steps to prepare GB dataset: https://github.com/osm-search/gb-postcode-data
  // * Direct from Nominatim: https://nominatim.org/data/gb_postcodes.csv.gz

  FieldIndices ukFieldIndices;
  ukFieldIndices.m_postcodeIndex = 0;
  ukFieldIndices.m_latIndex = 1;
  ukFieldIndices.m_longIndex = 2;
  ukFieldIndices.m_datasetCount = 3;

  GetPostcodes(filename, countryId, ukFieldIndices, "," /* separator */, true /* hasHeader */, infoGetter, valueMapping,
               keyValuePairs);
}

void GetUSPostcodes(std::string const & filename, storage::CountryId const & countryId,
                    storage::CountryInfoGetter const & infoGetter, std::vector<m2::PointD> & valueMapping,
                    std::vector<ZipIndexValue> & keyValuePairs)
{
  // * Get data source from here: https://simplemaps.com/data/us-zips
  //    "zip","lat","lng","city","state_id","state_name", ...
  // * From Nominatim (IDK the origins) https://nominatim.org/data/us_postcodes.csv.gz

  FieldIndices usFieldIndices;
  usFieldIndices.m_postcodeIndex = 0;
  usFieldIndices.m_latIndex = 1;
  usFieldIndices.m_longIndex = 2;
  usFieldIndices.m_datasetCount = 3;  // actually, there are more fields, but doesn't matter here

  // US dataset may contain complex json values, but doesn't matter for parsing first 3 entries.
  GetPostcodes(filename, countryId, usFieldIndices, "," /* separator */, true /* hasHeader */, infoGetter, valueMapping,
               keyValuePairs);
}

bool BuildPostcodePointsImpl(FilesContainerR & container, storage::CountryId const & country,
                             indexer::PostcodePointsDatasetType type, std::string const & datasetPath,
                             std::string const & tmpName, storage::CountryInfoGetter const & infoGetter,
                             Writer & writer)
{
  CHECK_EQUAL(writer.Pos(), 0, ());

  search::PostcodePoints::Header header;
  header.Serialize(writer);

  uint64_t bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_trieOffset = base::asserted_cast<uint32_t>(writer.Pos());

  std::vector<ZipIndexValue> postcodesKeyValuePairs;
  std::vector<m2::PointD> valueMapping;
  switch (type)
  {
  case indexer::PostcodePointsDatasetType::UK:
    GetUKPostcodes(datasetPath, country, infoGetter, valueMapping, postcodesKeyValuePairs);
    break;
  case indexer::PostcodePointsDatasetType::US:
    GetUSPostcodes(datasetPath, country, infoGetter, valueMapping, postcodesKeyValuePairs);
    break;
  default: UNREACHABLE();
  }

  if (postcodesKeyValuePairs.empty())
    return false;

  std::sort(postcodesKeyValuePairs.begin(), postcodesKeyValuePairs.end());

  {
    FileWriter tmpWriter(tmpName);
    SingleValueSerializer<typename ZipIndexValue::second_type> serializer;
    trie::Build<Writer, typename ZipIndexValue::first_type, SingleUint64Value>(tmpWriter, serializer,
                                                                               postcodesKeyValuePairs);
  }

  rw_ops::Reverse(FileReader(tmpName), writer);

  header.m_trieSize = base::asserted_cast<uint32_t>(writer.Pos() - header.m_trieOffset);

  bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_pointsOffset = base::asserted_cast<uint32_t>(writer.Pos());

  {
    search::CentersTableBuilder builder;

    builder.SetGeometryParams(feature::DataHeader(container).GetBounds());
    for (size_t i = 0; i < valueMapping.size(); ++i)
      builder.Put(base::asserted_cast<uint32_t>(i), valueMapping[i]);

    builder.Freeze(writer);
  }

  header.m_pointsSize = base::asserted_cast<uint32_t>(writer.Pos() - header.m_pointsOffset);
  auto const endOffset = writer.Pos();
  writer.Seek(0);
  header.Serialize(writer);
  writer.Seek(endOffset);
  return true;
}
}  // namespace

bool BuildPostcodePointsWithInfoGetter(std::string const & path, storage::CountryId const & country,
                                       PostcodePointsDatasetType type, std::string const & datasetPath,
                                       bool forceRebuild, storage::CountryInfoGetter const & infoGetter)
{
  auto const filename = base::JoinPath(path, country + DATA_FILE_EXTENSION);
  Platform & platform = GetPlatform();
  FilesContainerR readContainer(platform.GetReader(filename, "f"));
  if (readContainer.IsExist(POSTCODE_POINTS_FILE_TAG) && !forceRebuild)
    return true;

  auto const postcodesFilePath = filename + "." + POSTCODE_POINTS_FILE_TAG EXTENSION_TMP;
  // Temporary file used to reverse trie part of postcodes section.
  auto const trieTmpFilePath = filename + "." + POSTCODE_POINTS_FILE_TAG + "_trie" + EXTENSION_TMP;
  SCOPE_GUARD(postcodesFileGuard, std::bind(&FileWriter::DeleteFileX, postcodesFilePath));
  SCOPE_GUARD(trieTmpFileGuard, std::bind(&FileWriter::DeleteFileX, trieTmpFilePath));

  try
  {
    FileWriter writer(postcodesFilePath);
    if (!BuildPostcodePointsImpl(readContainer, storage::CountryId(country), type, datasetPath, trieTmpFilePath,
                                 infoGetter, writer))
    {
      // No postcodes for country.
      return true;
    }

    LOG(LINFO, ("Postcodes section size =", writer.Size()));
    FilesContainerW writeContainer(readContainer.GetFileName(), FileWriter::OP_WRITE_EXISTING);
    writeContainer.Write(postcodesFilePath, POSTCODE_POINTS_FILE_TAG);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file:", e.Msg()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error writing file:", e.Msg()));
    return false;
  }

  return true;
}

bool BuildPostcodePoints(std::string const & path, storage::CountryId const & country, PostcodePointsDatasetType type,
                         std::string const & datasetPath, bool forceRebuild)
{
  auto const & platform = GetPlatform();
  auto const infoGetter = storage::CountryInfoReader::CreateCountryInfoGetter(platform);
  CHECK(infoGetter, ());
  return BuildPostcodePointsWithInfoGetter(path, country, type, datasetPath, forceRebuild, *infoGetter);
}

}  // namespace indexer
