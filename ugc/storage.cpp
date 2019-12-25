#include "ugc/storage.hpp"
#include "ugc/index_migration/utility.hpp"
#include "ugc/serdes.hpp"
#include "ugc/serdes_json.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/ftraits.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/point_coding.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <utility>

#include "3party/Alohalytics/src/alohalytics.h"
#include "3party/jansson/myjansson.hpp"

using namespace std;
using namespace ugc;

namespace
{
string const kIndexFileName = "index.json";
string const kUGCUpdateFileName = "ugc.update.bin";
string const kTmpFileExtension = ".tmp";

using Sink = MemWriter<string>;

bool GetUGCFileSize(uint64_t & size)
{
  try
  {
    FileReader reader(Storage::GetUGCFilePath());
    size = reader.Size();
  }
  catch (FileReader::Exception const &)
  {
    return false;
  }

  return true;
}

void DeserializeIndexes(string const & jsonData, ugc::UpdateIndexes & res)
{
  if (jsonData.empty())
    return;

  try
  {
    ugc::DeserializerJsonV0 des(jsonData);
    des(res);
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LERROR, ("Exception while indexes deserialization. Reason:", e.what()));
    map<string, string> const stat = {
        {"error", "Cannot deserialize indexes. Content: " + jsonData}};
    alohalytics::Stats::Instance().LogEvent("UGC_File_error", stat);
    res.clear();
  }
}

void DeserializeUGCUpdate(vector<uint8_t> const & buf, ugc::UGCUpdate & dst)
{
  MemReaderWithExceptions r(buf.data(), buf.size());
  NonOwningReaderSource source(r);

  try
  {
    ugc::Deserialize(source, dst);
    return;
  }
  catch (ugc::BadBlob const & e)
  {
    LOG(LERROR, ("BadBlob exception while UGCUpdate deserialization. Reason:", e.what()));
    map<string, string> const stat = {
        {"error", "Cannot deserialize UGCUpdate. Bad blob exception. Content: " +
                      std::string(buf.cbegin(), buf.cend())}};
    alohalytics::Stats::Instance().LogEvent("UGC_File_error", stat);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Exception while UGCUpdate deserialization. Reason:", e.what()));
    map<string, string> const stat = {{"error", "Cannot deserialize UGCUpdate. Content: " +
                                                    std::string(buf.cbegin(), buf.cend())}};
    alohalytics::Stats::Instance().LogEvent("UGC_File_error", stat);
  }

  dst = {};
}

string SerializeIndexes(ugc::UpdateIndexes const & indexes)
{
  if (indexes.empty())
    return string();

  auto array = base::NewJSONArray();
  for (auto const & index : indexes)
  {
    string data;
    {
      Sink sink(data);
      ugc::SerializerJson<Sink> ser(sink);
      ser(index);
    }

    base::Json node(data);
    json_array_append_new(array.get(), node.get_deep_copy());
  }

  unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(array.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  return string(buffer.get());
}

bool SaveIndexes(ugc::UpdateIndexes const & indexes, std::string const & pathToTargetFile = "")
{
  if (indexes.empty())
    return false;

  auto const indexFilePath =
      pathToTargetFile.empty() ? Storage::GetIndexFilePath() : pathToTargetFile;
  auto const jsonData = SerializeIndexes(indexes);
  try
  {
    FileWriter w(indexFilePath);
    w.Write(jsonData.c_str(), jsonData.length());
  }
  catch (FileWriter::Exception const & exception)
  {
    LOG(LERROR, ("Exception while writing file:", indexFilePath, "reason:", exception.what()));
    base::DeleteFileX(indexFilePath);
    return false;
  }

  return true;
}

template <typename UGCUpdate>
ugc::Storage::SettingResult SetGenericUGCUpdate(UGCUpdate const & ugc,
                                                FeatureType & featureType,
                                                ugc::UpdateIndexes & indexes,
                                                size_t & numberOfDeleted,
                                                ugc::Version const version)
{
  if (!ugc.IsValid())
    return ugc::Storage::SettingResult::InvalidUGC;

  auto const mercator = feature::GetCenter(featureType);
  feature::TypesHolder th(featureType);
  th.SortBySpec();
  auto const optMatchingType = ftraits::UGC::GetType(th);
  CHECK(optMatchingType, ());
  auto const & c = classif();
  auto const type = c.GetIndexForType(th.GetBestType());
  for (auto & index : indexes)
  {
    if (type == index.m_type && mercator == index.m_mercator && !index.m_deleted)
    {
      index.m_deleted = true;
      ++numberOfDeleted;
      break;
    }
  }

  ugc::UpdateIndex index;
  uint64_t offset;
  if (!GetUGCFileSize(offset))
    offset = 0;

  auto const & id = featureType.GetID();
  index.m_mercator = mercator;
  index.m_type = type;
  index.m_matchingType = c.GetIndexForType(*optMatchingType);
  index.m_mwmName = id.GetMwmName();
  index.m_dataVersion = id.GetMwmVersion();
  index.m_featureId = id.m_index;
  index.m_offset = offset;
  index.m_version = ugc::IndexVersion::Latest;

  auto const ugcFilePath = Storage::GetUGCFilePath();
  try
  {
    FileWriter w(ugcFilePath, FileWriter::Op::OP_APPEND);
    Serialize(w, ugc, version);
  }
  catch (FileWriter::Exception const & exception)
  {
    LOG(LERROR, ("Exception while writing file:", ugcFilePath, "reason:", exception.what()));
    return ugc::Storage::SettingResult::WritingError;
  }

  indexes.emplace_back(move(index));

  return SaveIndexes(indexes) ? ugc::Storage::SettingResult::Success
                              : ugc::Storage::SettingResult::WritingError;
}

void FindZombieObjects(size_t indexesCount)
{
  auto const ugcFilePath = Storage::GetUGCFilePath();
  vector<uint8_t> ugcFileContent;
  try
  {
    FileReader r(ugcFilePath);
    ugcFileContent = r.ReadAsBytes();
  }
  catch (FileReader::Exception const & exception)
  {
    ugcFileContent.clear();
  }

  uint32_t ugcCount = 0;

  MemReaderWithExceptions r(ugcFileContent.data(), ugcFileContent.size());
  NonOwningReaderSource source(r);
  ugc::UGCUpdate unused;
  try
  {
    while (source.Size() != 0)
    {
      ++ugcCount;
      ugc::Deserialize(source, unused);
    }
  }
  catch (RootException const & e)
  {
    auto const error = "Cannot deserialize ugc.update.bin file during zombie objects search";
    LOG(LERROR, (error));

    map<string, string> errorParams = {{"error", error}};
    alohalytics::Stats::Instance().LogEvent("UGC_File_error", errorParams);
    return;
  }

  if (indexesCount == ugcCount)
    return;

  auto const zombieCount = ugcCount - indexesCount;
  LOG(LERROR, ("Zombie objects are detected. ", zombieCount, "zombie objects are found."));

  map<string, string> errorParams = {{"error", "zombie: " + strings::to_string(zombieCount)}};
  alohalytics::Stats::Instance().LogEvent("UGC_File_error", errorParams);
}
}  // namespace

namespace ugc
{
// static
string Storage::GetUGCFilePath()
{
  return base::JoinPath(GetPlatform().SettingsDir(), kUGCUpdateFileName);
}

// static
string Storage::GetIndexFilePath()
{
  return base::JoinPath(GetPlatform().SettingsDir(), kIndexFileName);
}

UGCUpdate Storage::GetUGCUpdate(FeatureID const & id) const
{
  if (m_indexes.empty())
    return {};

  auto const index = FindIndex(id);

  if (index == m_indexes.end())
    return {};

  auto const offset = index->m_offset;
  auto const pos = static_cast<size_t>(distance(m_indexes.begin(), index));
  auto const size = static_cast<size_t>(UGCSizeAtPos(pos));
  vector<uint8_t> buf;
  buf.resize(size);
  auto const ugcFilePath = GetUGCFilePath();
  try
  {
    FileReader r(ugcFilePath);
    r.Read(offset, buf.data(), size);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LERROR, ("Exception while reading file:", ugcFilePath, "reason:", exception.what()));
    return {};
  }

  UGCUpdate update;
  DeserializeUGCUpdate(buf, update);
  return update;
}

Storage::SettingResult Storage::SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc)
{
  auto const feature = GetFeature(id);
  return SetGenericUGCUpdate(ugc, *feature, m_indexes, m_numberOfDeleted, Version::V1);
}

void Storage::Load()
{
  string data;
  auto const indexFilePath = GetIndexFilePath();
  try
  {
    FileReader r(indexFilePath);
    r.ReadAsString(data);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", indexFilePath, "reason:", exception.what()));
    return;
  }

  if (data.empty())
  {
    ASSERT(false, ());
    map<string, string> const stat = {{"error", "empty index file"}};
    alohalytics::Stats::Instance().LogEvent("UGC_File_error", stat);
    return;
  }

  DeserializeIndexes(data, m_indexes);
  if (m_indexes.empty())
    return;

  optional<IndexVersion> version;
  for (auto const & i : m_indexes)
  {
    if (i.m_deleted)
    {
      ++m_numberOfDeleted;
      continue;
    }

    if (!version)
      version = i.m_version;
    else
      CHECK_EQUAL(static_cast<uint8_t>(*version), static_cast<uint8_t>(i.m_version), ("Inconsistent index:", data));
  }

  if (version && *version != IndexVersion::Latest)
    Migrate(indexFilePath);
}

void Storage::Migrate(string const & indexFilePath)
{
  CHECK(!m_indexes.empty(), ());

  string const suffix = ".v0";
  auto const ugcFilePath = GetUGCFilePath();

  // Backup existing files
  auto const v0IndexFilePath = indexFilePath + suffix;
  auto const v0UGCFilePath = ugcFilePath + suffix;
  if (!base::CopyFileX(indexFilePath, v0IndexFilePath))
  {
    LOG(LERROR, ("Can't backup UGC index file"));
    return;
  }

  if (!base::CopyFileX(ugcFilePath, v0UGCFilePath))
  {
    base::DeleteFileX(v0IndexFilePath);
    LOG(LERROR, ("Can't backup UGC update file"));
    return;
  }

  switch (migration::Migrate(m_indexes))
  {
  case migration::Result::NeedDefragmentation:
    LOG(LINFO, ("Need defragmentation after successful UGC index migration"));
    DefragmentationImpl(true /* force */);
    // fallthrough
  case migration::Result::Success:
    if (!SaveIndexes(m_indexes, indexFilePath))
    {
      base::DeleteFileX(indexFilePath);
      base::DeleteFileX(ugcFilePath);
      base::RenameFileX(v0UGCFilePath, ugcFilePath);
      base::RenameFileX(v0IndexFilePath, indexFilePath);
      m_indexes.clear();
      LOG(LERROR, ("Can't save UGC index after migration"));
      return;
    }

    LOG(LINFO, ("UGC index migration successful"));
    break;
  }
}

UpdateIndexes::const_iterator Storage::FindIndex(FeatureID const & id) const
{
  auto const feature = GetFeature(id);
  auto const mercator = feature::GetCenter(*feature);
  feature::TypesHolder th(*feature);
  th.SortBySpec();

  return FindIndex(th.GetBestType(), mercator);
}

UpdateIndexes::const_iterator Storage::FindIndex(uint32_t bestType, m2::PointD const & point) const
{
  auto const & c = classif();
  auto const typeIndex = c.GetIndexForType(bestType);

  return find_if(
    m_indexes.begin(), m_indexes.end(), [typeIndex, &point](UpdateIndex const & index) -> bool {
      return typeIndex == index.m_type && point.EqualDxDy(index.m_mercator, kMwmPointAccuracy) &&
             !index.m_deleted;
    });
}

void Storage::Defragmentation()
{
  DefragmentationImpl(false /* force */);
}

void Storage::DefragmentationImpl(bool force)
{
  auto const indexesSize = m_indexes.size();
  if (!force && m_numberOfDeleted < indexesSize / 2)
    return;

  auto const ugcFilePath = GetUGCFilePath();
  auto const tmpUGCFilePath = ugcFilePath + kTmpFileExtension;

  try
  {
    FileReader r(ugcFilePath);
    FileWriter w(tmpUGCFilePath, FileWriter::Op::OP_APPEND);
    uint64_t actualOffset = 0;
    for (size_t i = 0; i < indexesSize; ++i)
    {
      auto & index = m_indexes[i];
      if (index.m_deleted)
        continue;

      auto const offset = index.m_offset;
      auto const size = static_cast<size_t>(UGCSizeAtPos(i));
      vector<uint8_t> buf;
      buf.resize(size);
      r.Read(offset, buf.data(), size);
      w.Write(buf.data(), size);
      index.m_offset = actualOffset;
      actualOffset += size;
    }
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LERROR, ("Exception while reading file:", ugcFilePath, "reason:", exception.what()));
    return;
  }
  catch (FileWriter::Exception const & exception)
  {
    LOG(LERROR, ("Exception while writing file:", tmpUGCFilePath, "reason:", exception.what()));
    return;
  }

  base::EraseIf(m_indexes, [](UpdateIndex const & i) -> bool { return i.m_deleted; });
  CHECK(base::DeleteFileX(ugcFilePath), ());
  CHECK(base::RenameFileX(tmpUGCFilePath, ugcFilePath), ());

  m_numberOfDeleted = 0;
}

string Storage::GetUGCToSend() const
{
  if (m_indexes.empty())
    return string();

  auto array = base::NewJSONArray();
  auto const indexesSize = m_indexes.size();
  auto const ugcFilePath = GetUGCFilePath();
  FileReader r(ugcFilePath);
  vector<uint8_t> buf;
  for (size_t i = 0; i < indexesSize; ++i)
  {
    buf.clear();
    auto const & index = m_indexes[i];
    if (index.m_synchronized || index.m_deleted)
      continue;

    auto const offset = index.m_offset;
    auto const bufSize = static_cast<size_t>(UGCSizeAtPos(i));
    buf.resize(bufSize);
    try
    {
      r.Read(offset, buf.data(), bufSize);
    }
    catch (FileReader::Exception const & exception)
    {
      LOG(LERROR, ("Exception while reading file:", ugcFilePath, "reason:", exception.what()));
      return string();
    }

    UGCUpdate update;
    DeserializeUGCUpdate(buf, update);

    if (update.IsEmpty())
      continue;

    string data;
    {
      Sink sink(data);
      SerializerJson<Sink> ser(sink);
      ser(update);
    }

    base::Json serializedUgc(data);
    auto embeddedNode = base::NewJSONObject();
    ToJSONObject(*embeddedNode.get(), "data_version", index.m_dataVersion);
    ToJSONObject(*embeddedNode.get(), "mwm_name", index.m_mwmName);
    ToJSONObject(*embeddedNode.get(), "feature_id", index.m_featureId);
    auto const & c = classif();
    ToJSONObject(*embeddedNode.get(), "feature_type",
                 c.GetReadableObjectName(c.GetTypeForIndex(index.m_matchingType)));
    ToJSONObject(*serializedUgc.get(), "feature", *embeddedNode.release());
    json_array_append_new(array.get(), serializedUgc.get_deep_copy());
  }

  if (json_array_size(array.get()) == 0)
    return string();

  auto reviewsNode = base::NewJSONObject();
  ToJSONObject(*reviewsNode.get(), "reviews", *array.release());

  unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(reviewsNode.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  return string(buffer.get());
}

size_t Storage::GetNumberOfUnsynchronized() const
{
  size_t numberOfUnsynchronized = 0;
  for (auto const & i : m_indexes)
  {
    if (!i.m_deleted && !i.m_synchronized)
      ++numberOfUnsynchronized;
  }

  return numberOfUnsynchronized;
}

bool Storage::HasUGCForPlace(uint32_t bestType, m2::PointD const & point) const
{
  return FindIndex(bestType, point) != m_indexes.cend();
}

void Storage::Validate() const
{
  FindZombieObjects(m_indexes.size());
}

void Storage::MarkAllAsSynchronized()
{
  if (m_indexes.empty())
    return;

  size_t numberOfUnsynchronized = 0;
  for (auto & index : m_indexes)
  {
    if (!index.m_synchronized)
    {
      index.m_synchronized = true;
      numberOfUnsynchronized++;
    }
  }

  if (numberOfUnsynchronized == 0)
    return;

  auto const indexPath = GetIndexFilePath();
  base::DeleteFileX(indexPath);
  SaveIndexes(m_indexes);
}

uint64_t Storage::UGCSizeAtPos(size_t const pos) const
{
  CHECK(!m_indexes.empty(), ());
  auto const indexesSize = m_indexes.size();
  CHECK_LESS(pos, indexesSize, ());
  auto const offset = m_indexes[pos].m_offset;
  uint64_t nextOffset;
  if (pos == indexesSize - 1)
    CHECK(GetUGCFileSize(nextOffset), ());
  else
    nextOffset = m_indexes[pos + 1].m_offset;

  CHECK_GREATER(nextOffset, offset, ());
  return nextOffset - offset;
}

unique_ptr<FeatureType> Storage::GetFeature(FeatureID const & id) const
{
  CHECK(id.IsValid(), ());
  FeaturesLoaderGuard guard(m_dataSource, id.m_mwmId);
  auto feature = guard.GetOriginalOrEditedFeatureByIndex(id.m_index);
  feature->ParseGeometry(FeatureType::BEST_GEOMETRY);
  if (feature->GetGeomType() == feature::GeomType::Area)
    feature->ParseTriangles(FeatureType::BEST_GEOMETRY);
  CHECK(feature, ());
  return feature;
}

// Testing
Storage::SettingResult Storage::SetUGCUpdateForTesting(FeatureID const & id,
                                                       v0::UGCUpdate const & ugc)
{
  auto const feature = GetFeature(id);
  return SetGenericUGCUpdate(ugc, *feature, m_indexes, m_numberOfDeleted, Version::V0);
}

void Storage::LoadForTesting(std::string const & testIndexFilePath)
{
  string data;
  try
  {
    FileReader r(testIndexFilePath);
    r.ReadAsString(data);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LWARNING, (exception.what()));
    return;
  }

  CHECK(!data.empty(), ());
  DeserializeIndexes(data, m_indexes);

  if (m_indexes.front().m_version != IndexVersion::Latest)
    Migrate(testIndexFilePath);
}

bool Storage::SaveIndexForTesting(std::string const & testIndexFilePath) const
{
  return SaveIndexes(m_indexes, testIndexFilePath);
}
}  // namespace ugc

namespace lightweight
{
namespace impl
{
size_t GetNumberOfUnsentUGC()
{
  auto const indexFilePath = Storage::GetIndexFilePath();
  if (!Platform::IsFileExistsByFullPath(indexFilePath))
    return 0;

  string data;
  try
  {
    FileReader r(indexFilePath);
    r.ReadAsString(data);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", indexFilePath, "reason:", exception.what()));
    return 0;
  }

  ugc::UpdateIndexes index;
  DeserializeIndexes(data, index);
  size_t number = 0;
  for (auto const & i : index)
  {
    if (!i.m_deleted && !i.m_synchronized)
      ++number;
  }

  return number;
}
}  // namespace impl
}  // namespace lightweight
