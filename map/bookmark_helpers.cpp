#include "map/bookmark_helpers.hpp"

#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/feature_utils.hpp"

#include "platform/preferred_languages.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

namespace
{
void ValidateKmlData(std::unique_ptr<kml::FileData> & data)
{
  if (!data)
    return;

  for (auto & t : data->m_tracksData)
  {
    if (t.m_layers.empty())
      t.m_layers.emplace_back(kml::KmlParser::GetDefaultTrackLayer());
  }
}
}  // namespace

std::unique_ptr<kml::FileData> LoadKmlFile(std::string const & file, bool useBinary)
{
  std::unique_ptr<kml::FileData> kmlData;
  try
  {
    kmlData = LoadKmlData(FileReader(file), useBinary);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", useBinary ? "binary" : "text", "loading failure:", e.what()));
    kmlData.reset();
  }
  if (kmlData == nullptr)
    LOG(LWARNING, ("Loading bookmarks failed, file", file));
  return kmlData;
}

std::unique_ptr<kml::FileData> LoadKmlData(Reader const & reader, bool useBinary)
{
  auto data = std::make_unique<kml::FileData>();
  try
  {
    if (useBinary)
    {
      kml::binary::DeserializerKml des(*data);
      des.Deserialize(reader);
    }
    else
    {
      kml::DeserializerKml des(*data);
      des.Deserialize(reader);
    }
    ValidateKmlData(data);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LWARNING, ("KML", useBinary ? "binary" : "text", "reading failure:", e.what()));
    return nullptr;
  }
  catch (kml::binary::DeserializerKml::DeserializeException const & e)
  {
    LOG(LWARNING, ("KML binary deserialization failure:", e.what()));
    return nullptr;
  }
  catch (kml::DeserializerKml::DeserializeException const & e)
  {
    LOG(LWARNING, ("KML text deserialization failure:", e.what()));
    return nullptr;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", useBinary ? "binary" : "text", "loading failure:", e.what()));
    return nullptr;
  }
  return data;
}

bool SaveKmlFile(kml::FileData & kmlData, std::string const & file, bool useBinary)
{
  bool success;
  try
  {
    FileWriter writer(file);
    success = SaveKmlData(kmlData, writer, useBinary);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", useBinary ? "binary" : "text", "saving failure:", e.what()));
    success = false;
  }
  if (!success)
    LOG(LWARNING, ("Saving bookmarks failed, file", file));
  return success;
}

bool SaveKmlData(kml::FileData & kmlData, Writer & writer, bool useBinary)
{
  try
  {
    if (useBinary)
    {
      kml::binary::SerializerKml ser(kmlData);
      ser.Serialize(writer);
    }
    else
    {
      kml::SerializerKml ser(kmlData);
      ser.Serialize(writer);
    }
  }
  catch (Writer::Exception const & e)
  {
    LOG(LWARNING, ("KML", useBinary ? "binary" : "text", "writing failure:", e.what()));
    return false;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", useBinary ? "binary" : "text", "serialization failure:", e.what()));
    return false;
  }
  return true;
}

void ResetIds(kml::FileData & kmlData)
{
  kmlData.m_categoryData.m_id = kml::kInvalidMarkGroupId;
  for (auto & bmData : kmlData.m_bookmarksData)
    bmData.m_id = kml::kInvalidMarkId;
  for (auto & trackData : kmlData.m_tracksData)
    trackData.m_id = kml::kInvalidTrackId;
}

void SaveFeatureTypes(feature::TypesHolder const & types, kml::BookmarkData & bmData)
{
  feature::TypesHolder copy(types);
  copy.SortBySpec();
  bmData.m_featureTypes.assign(copy.begin(), copy.end());
}

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name)
{
  if (name.size() == 1)
    return name.begin()->second;

  StringUtf8Multilang nameMultilang;
  for (auto const & pair : name)
    nameMultilang.AddString(pair.first, pair.second);

  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());

  std::string preferredName;
  if (feature::GetPreferredName(nameMultilang, deviceLang, preferredName))
    return preferredName;

  return {};
}

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name, feature::RegionData const & regionData)
{
  if (name.size() == 1)
    return name.begin()->second;

  StringUtf8Multilang nameMultilang;
  for (auto const & pair : name)
    nameMultilang.AddString(pair.first, pair.second);

  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());

  std::string preferredName;
  feature::GetReadableName(regionData, nameMultilang, deviceLang, false /* allowTranslit */, preferredName);
  return preferredName;
}

std::string GetLocalizedBookmarkType(std::vector<uint32_t> const & types)
{
  if (types.empty())
    return {};

  CategoriesHolder const & categories = GetDefaultCategories();
  return categories.GetReadableFeatureType(types.front(), categories.MapLocaleToInteger(languages::GetCurrentOrig()));
}

std::string GetPreferredBookmarkName(kml::BookmarkData const & bmData)
{
  std::string name = GetPreferredBookmarkStr(bmData.m_customName);
  if (name.empty())
    name = GetPreferredBookmarkStr(bmData.m_name);
  if (name.empty())
    name = GetLocalizedBookmarkType(bmData.m_featureTypes);
  return name;
}
