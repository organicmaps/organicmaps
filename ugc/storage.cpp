#include "ugc/storage.hpp"
#include "ugc/serdes.hpp"
#include "ugc/serdes_json.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/index.hpp"

#include "platform/platform.hpp"

#include <algorithm>
#include <utility>

#include "3party/jansson/myjansson.hpp"

namespace ugc
{
using namespace std;
namespace
{
string const kIndexFileName = "index.json";
string const kUGCUpdateFileName = "ugc.update.bin";
string const kTmpFileExtension = ".tmp";

char const kOffsetKey[] = "offset";
char const kXKey[] = "x";
char const kYKey[] = "y";
char const kTypeKey[] = "type";
char const kIsDeletedKey[] = "is_deleted";
char const kIsSynchronizedKey[] = "is_synchronized";
char const kMwmNameKey[] = "mwm_name";
char const kDataVersionKey[] = "data_version";
char const kFeatureIdKey[] = "feature_id";
char const kFeatureKey[] = "feature";

string GetUGCFilePath()
{
  return GetPlatform().WritableDir() + kUGCUpdateFileName;
}

string GetIndexFilePath()
{
  return GetPlatform().WritableDir() + kIndexFileName;
}

bool GetUGCFileSize(uint64_t & size)
{
  return GetPlatform().GetFileSizeByName(kUGCUpdateFileName, size);
}

void DeserializeUGCIndex(string const & jsonData, vector<Storage::UGCIndex> & res, size_t & numberOfDeleted)
{
  if (jsonData.empty())
    return;

  my::Json root(jsonData.c_str());
  if (!root.get() || !json_is_array(root.get()))
    return;

  size_t const size = json_array_size(root.get());
  if (size == 0)
    return;

  for (size_t i = 0; i < size; ++i)
  {
    auto node = json_array_get(root.get(), i);
    if (!node)
      return;

    Storage::UGCIndex index;
    double x, y;
    FromJSONObject(node, kXKey, x);
    FromJSONObject(node, kYKey, y);
    index.m_mercator = {x, y};

    uint32_t type;
    FromJSONObject(node, kTypeKey, type);
    index.m_type = type;

    uint64_t offset;
    FromJSONObject(node, kOffsetKey, offset);
    index.m_offset = offset;

    bool isDeleted;
    FromJSONObject(node, kIsDeletedKey, isDeleted);
    index.m_isDeleted = isDeleted;
    if (isDeleted)
      numberOfDeleted++;

    bool isSynchronized;
    FromJSONObject(node, kIsSynchronizedKey, isSynchronized);
    index.m_isSynchronized = isDeleted;

    string mwmName;
    FromJSONObject(node, kMwmNameKey, mwmName);
    index.m_mwmName = mwmName;

    string mwmVersion;
    FromJSONObject(node, kDataVersionKey, mwmVersion);
    index.m_dataVersion = mwmName;

    uint32_t featureId;
    FromJSONObject(node, kFeatureIdKey, featureId);
    index.m_featureId = featureId;

    res.emplace_back(move(index));
  }
}

string SerializeUGCIndex(vector<Storage::UGCIndex> const & indexes)
{
  if (indexes.empty())
    return string();

  auto array = my::NewJSONArray();
  for (auto const & i : indexes)
  {
    auto node = my::NewJSONObject();
    auto const & mercator = i.m_mercator;
    ToJSONObject(*node, kXKey, mercator.x);
    ToJSONObject(*node, kYKey, mercator.y);
    ToJSONObject(*node, kTypeKey, i.m_type);
    ToJSONObject(*node, kOffsetKey, i.m_offset);
    ToJSONObject(*node, kIsDeletedKey, i.m_isDeleted);
    ToJSONObject(*node, kIsSynchronizedKey, i.m_isSynchronized);
    ToJSONObject(*node, kMwmNameKey, i.m_mwmName);
    ToJSONObject(*node, kDataVersionKey, i.m_dataVersion);
    ToJSONObject(*node, kFeatureIdKey, i.m_featureId);
    json_array_append_new(array.get(), node.release());
  }

  unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(array.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  return string(buffer.get());
}

void SerializeUGCUpdate(json_t * node, UGCUpdate const & update, Storage::UGCIndex const & index)
{
  vector<char> data;
  using Sink = MemWriter<vector<char>>;
  Sink sink(data);
  SerializerJson<Sink> ser(sink);
  ser(update);

  my::Json serializedUgc(data.data());
  node = serializedUgc.get();
  auto embeddedNode = my::NewJSONObject();
  ToJSONObject(*embeddedNode, kDataVersionKey, index.m_dataVersion);
  ToJSONObject(*embeddedNode, kMwmNameKey, index.m_mwmName);
  ToJSONObject(*embeddedNode, kFeatureIdKey, index.m_featureId);
  ToJSONObject(*node, kFeatureKey, *embeddedNode);
  embeddedNode.release();
}
}  // namespace

Storage::Storage(Index const & index)
  : m_index(index)
{
  Load();
}

UGCUpdate Storage::GetUGCUpdate(FeatureID const & id) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (m_UGCIndexes.empty())
    return {};

  auto const feature = GetOriginalFeature(id);
  CHECK(feature, ());
  auto const & mercator = feature->GetCenter();
  feature::TypesHolder th(*feature);
  th.SortBySpec();
  auto const type = th.GetBestType();

  auto const index = find_if(m_UGCIndexes.begin(), m_UGCIndexes.end(), [type, &mercator](UGCIndex const & index) -> bool
  {
    return type == index.m_type && mercator == index.m_mercator && !index.m_isDeleted;
  });

  if (index == m_UGCIndexes.end())
    return {};

  auto const offset = index->m_offset;
  auto const nextIndex = index + 1;
  uint64_t nextOffset;
  if (nextIndex == m_UGCIndexes.end())
    CHECK(GetPlatform().GetFileSizeByName(kUGCUpdateFileName, nextOffset), ());
  else
    nextOffset = nextIndex->m_offset;

  auto const size = nextOffset - offset;
  vector<uint8_t> buf;
  auto const ugcFilePath = GetUGCFilePath();
  try
  {
    FileReader r(ugcFilePath);
    r.Read(offset, buf.data(), size);
  }
  catch (RootException const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", ugcFilePath));
    return {};
  }

  MemReader r(buf.data(), buf.size());
  NonOwningReaderSource source(r);
  UGCUpdate update;
  Deserialize(source, update);
  return update;
}

void Storage::SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto const feature = GetOriginalFeature(id);
  CHECK(feature, ());

  auto const & mercator = feature->GetCenter();
  feature::TypesHolder th(*feature);
  th.SortBySpec();
  auto const type = th.GetBestType();
  for (auto & index : m_UGCIndexes)
  {
    if (type == index.m_type && mercator == index.m_mercator && !index.m_isDeleted)
    {
      index.m_isDeleted = true;
      m_numberOfDeleted++;
      break;
    }
  }
  // TODO: Call Defragmentation().

  auto const ugcFilePath = GetUGCFilePath();
  try
  {
    FileWriter w(ugcFilePath);
    Serialize(w, ugc);
  }
  catch (RootException const & exception)
  {
    LOG(LWARNING, ("Exception while writing file:", ugcFilePath));
    return;
  }

  UGCIndex index;
  index.m_mercator = mercator;
  uint64_t offset;
  if (GetUGCFileSize(offset))
    offset = 0;

  index.m_type = type;
  index.m_mwmName = id.GetMwmName();
  index.m_dataVersion = id.GetMwmVersion();
  index.m_featureId = id.m_index;
  index.m_offset = offset;
  m_UGCIndexes.emplace_back(move(index));
}

void Storage::Load()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  string data;
  auto const indexFilePath = GetIndexFilePath();
  try
  {
    FileReader r(indexFilePath);
    r.ReadAsString(data);
  }
  catch (RootException const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", indexFilePath));
    return;
  }

  DeserializeUGCIndex(data, m_UGCIndexes, m_numberOfDeleted);
  sort(m_UGCIndexes.begin(), m_UGCIndexes.end(), [](UGCIndex const & lhs, UGCIndex const & rhs) -> bool {
    return lhs.m_offset < rhs.m_offset;
  });
}

void Storage::SaveIndex() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto const jsonData = SerializeUGCIndex(m_UGCIndexes);
  auto const indexFilePath = GetIndexFilePath();
  try
  {
    FileWriter w(indexFilePath);
    w.Write(jsonData.c_str(), jsonData.length());
  }
  catch (RootException const & exception)
  {
    LOG(LWARNING, ("Exception while writing file:", indexFilePath));
  }
}

void Storage::Defragmentation()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  auto const indexesSize = m_UGCIndexes.size();
  if (m_numberOfDeleted < indexesSize / 2)
    return;

  auto const ugcFilePath = GetUGCFilePath();
  auto const tmpUGCFilePath = ugcFilePath + kTmpFileExtension;
  FileReader r(ugcFilePath);
  vector<uint8_t> buf;
  for (size_t i = 0; i < indexesSize; ++i)
  {
    auto const & index = m_UGCIndexes[i];
    if (index.m_isDeleted)
      continue;

    auto const offset = index.m_offset;
    uint64_t nextOffset;
    if (i == indexesSize - 1)
      GetUGCFileSize(nextOffset);
    else
      nextOffset = m_UGCIndexes[i + 1].m_offset;

    auto const bufSize = nextOffset - offset;
    try
    {
      r.Read(offset, buf.data(), bufSize);
    }
    catch (RootException const & excpetion)
    {
      LOG(LWARNING, ("Exception while reading file:", ugcFilePath));
      return;
    }
  }

  try
  {
    FileWriter w(tmpUGCFilePath);
    w.Write(buf.data(), buf.size());
  }
  catch (RootException const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", tmpUGCFilePath));
    return;
  }

  m_UGCIndexes.erase(remove_if(m_UGCIndexes.begin(), m_UGCIndexes.end(), [](UGCIndex const & i) -> bool {
    return i.m_isDeleted;
  }));

  CHECK(my::DeleteFileX(ugcFilePath), ());
  CHECK(my::RenameFileX(tmpUGCFilePath, ugcFilePath), ());

  m_numberOfDeleted = 0;
}

string Storage::GetUGCToSend() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (m_UGCIndexes.empty())
    return string();

  auto array = my::NewJSONArray();
  auto const indexesSize = m_UGCIndexes.size();
  auto const ugcFilePath = GetUGCFilePath();
  FileReader r(ugcFilePath);
  vector<uint8_t> buf;
  for (size_t i = 0; i < indexesSize; ++i)
  {
    buf.clear();
    auto const & index = m_UGCIndexes[i];
    if (index.m_isSynchronized)
      continue;

    auto const offset = index.m_offset;
    uint64_t nextOffset;
    if (i == indexesSize - 1)
      CHECK(GetUGCFileSize(nextOffset), ());
    else
      nextOffset = m_UGCIndexes[i + 1].m_offset;

    auto const bufSize = nextOffset - offset;
    try
    {
      r.Read(offset, buf.data(), bufSize);
    }
    catch (RootException const & excpetion)
    {
      LOG(LWARNING, ("Exception while reading file:", ugcFilePath));
      return string();
    }

    MemReader r(buf.data(), buf.size());
    NonOwningReaderSource source(r);
    UGCUpdate update;
    Deserialize(source, update);

    auto node = my::NewJSONObject();
    SerializeUGCUpdate(node.get(), update, index);
    json_array_append_new(array.get(), node.release());
  }

  unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(array.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  return string(buffer.get());
}

void Storage::MarkAllAsSynchronized()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  if (m_UGCIndexes.empty())
    return;

  for (auto & index : m_UGCIndexes)
    index.m_isSynchronized = true;

  auto const indexPath = GetIndexFilePath();
  my::DeleteFileX(indexPath);
  SaveIndex();
}

unique_ptr<FeatureType> Storage::GetOriginalFeature(FeatureID const & id) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  CHECK(id.IsValid(), ());

  auto feature = make_unique<FeatureType>();
  Index::FeaturesLoaderGuard const guard(m_index, id.m_mwmId);
  if (!guard.GetOriginalFeatureByIndex(id.m_index, *feature))
    return unique_ptr<FeatureType>();

  feature->ParseEverything();
  return feature;
}
}  // namespace ugc

