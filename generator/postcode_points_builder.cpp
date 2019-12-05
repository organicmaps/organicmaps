#include "generator/postcode_points_builder.hpp"

#include "search/postcode_points.hpp"
#include "search/search_index_values.hpp"
#include "search/search_trie.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_builder.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage_defines.hpp"

#include "platform/platform.hpp"

#include "coding/map_uint32_to_val.hpp"
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

#include <cstdint>
#include <fstream>
#include <utility>
#include <vector>

#include "defines.hpp"

using namespace std;

namespace
{
template <typename Key, typename Value>
void GetUKPostcodes(string const & filename, storage::CountryId const & countryId,
                    storage::CountryInfoGetter & infoGetter, vector<m2::PointD> & valueMapping,
                    vector<pair<Key, Value>> & keyValuePairs)
{
  // Original dataset uses UK National Grid UTM coordinates.
  // It was converted to WGS84 by https://pypi.org/project/OSGridConverter/.
  size_t constexpr kPostcodeIndex = 0;
  size_t constexpr kLatIndex = 1;
  size_t constexpr kLongIndex = 2;
  size_t constexpr kDatasetCount = 3;

  ifstream data;
  data.exceptions(fstream::failbit | fstream::badbit);
  data.open(filename);
  data.exceptions(fstream::badbit);

  string line;
  size_t index = 0;
  while (getline(data, line))
  {
    vector<string> fields;
    strings::ParseCSVRow(line, ',', fields);
    CHECK_EQUAL(fields.size(), kDatasetCount, (line));

    double lat;
    CHECK(strings::to_double(fields[kLatIndex], lat), ());

    double lon;
    CHECK(strings::to_double(fields[kLongIndex], lon), ());

    auto const p = mercator::FromLatLon(lat, lon);

    vector<storage::CountryId> countries;
    infoGetter.GetRegionsCountryId(p, countries, 200.0 /* lookupRadiusM */);
    if (find(countries.begin(), countries.end(), countryId) == countries.end())
      continue;

    // UK postcodes formats are: aana naa, ana naa, an naa, ann naa, aan naa, aann naa.

    auto postcode = fields[kPostcodeIndex];
    // Do not index outer postcodes.
    if (postcode.size() < 5)
      continue;

    // Space is skipped in dataset for |aana naa| and |aann naa| to make it fit 7 symbols in csv.
    // Let's fix it here.
    if (postcode.find(' ') == string::npos)
      postcode.insert(static_cast<size_t>(postcode.size() - 3), " ");

    CHECK_EQUAL(valueMapping.size(), index, ());
    valueMapping.push_back(p);
    keyValuePairs.emplace_back(search::NormalizeAndSimplifyString(postcode), Value(index));
    ++index;
  }
}

bool BuildPostcodePointsImpl(FilesContainerR & container, storage::CountryId const & country,
                             string const & dataset, string const & tmpName,
                             storage::CountryInfoGetter & infoGetter, Writer & writer)
{
  using Key = strings::UniString;
  using Value = Uint64IndexValue;

  CHECK_EQUAL(writer.Pos(), 0, ());

  search::PostcodePoints::Header header;
  header.Serialize(writer);

  uint64_t bytesWritten = writer.Pos();
  coding::WritePadding(writer, bytesWritten);

  header.m_trieOffset = base::asserted_cast<uint32_t>(writer.Pos());

  vector<pair<Key, Value>> ukPostcodesKeyValuePairs;
  vector<m2::PointD> valueMapping;
  GetUKPostcodes(dataset, country, infoGetter, valueMapping, ukPostcodesKeyValuePairs);

  if (ukPostcodesKeyValuePairs.empty())
    return false;

  sort(ukPostcodesKeyValuePairs.begin(), ukPostcodesKeyValuePairs.end());

  {
    FileWriter tmpWriter(tmpName);
    SingleValueSerializer<Value> serializer;
    trie::Build<Writer, Key, SingleUint64Value, SingleValueSerializer<Value>>(
        tmpWriter, serializer, ukPostcodesKeyValuePairs);
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

namespace indexer
{
bool BuildPostcodePointsWithInfoGetter(string const & path, string const & country,
                                       string const & datasetPath, bool forceRebuild,
                                       storage::CountryInfoGetter & infoGetter)
{
  auto const filename = base::JoinPath(path, country + DATA_FILE_EXTENSION);
  if (filename == WORLD_FILE_NAME || filename == WORLD_COASTS_FILE_NAME)
    return true;

  Platform & platform = GetPlatform();
  FilesContainerR readContainer(platform.GetReader(filename, "f"));
  if (readContainer.IsExist(POSTCODE_POINTS_FILE_TAG) && !forceRebuild)
    return true;

  string const postcodesFilePath = filename + "." + POSTCODE_POINTS_FILE_TAG EXTENSION_TMP;
  // Temporary file used to reverse trie part of postcodes section.
  string const trieTmpFilePath =
      filename + "." + POSTCODE_POINTS_FILE_TAG + "_trie" + EXTENSION_TMP;
  SCOPE_GUARD(postcodesFileGuard, bind(&FileWriter::DeleteFileX, postcodesFilePath));
  SCOPE_GUARD(trieTmpFileGuard, bind(&FileWriter::DeleteFileX, trieTmpFilePath));

  try
  {
    FileWriter writer(postcodesFilePath);
    if (!BuildPostcodePointsImpl(readContainer, storage::CountryId(country), datasetPath,
                                 trieTmpFilePath, infoGetter, writer))
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

bool BuildPostcodePoints(string const & path, string const & country, string const & datasetPath,
                         bool forceRebuild)
{
  auto const & platform = GetPlatform();
  auto infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(platform);
  CHECK(infoGetter, ());
  return BuildPostcodePointsWithInfoGetter(path, country, datasetPath, forceRebuild, *infoGetter);
}

}  // namespace indexer
