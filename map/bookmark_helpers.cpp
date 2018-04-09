#include "map/bookmark_helpers.hpp"

#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

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
      kml::binary::DeserializerKml des(*data.get());
      des.Deserialize(reader);
    }
    else
    {
      kml::DeserializerKml des(*data.get());
      des.Deserialize(reader);
    }
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
  bool success = false;
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

void SaveFeatureInfo(StringUtf8Multilang const & name, feature::TypesHolder const & types, kml::BookmarkData & bmData)
{
  bmData.m_featureTypes.assign(types.begin(), types.end());

  name.ForEach([&bmData](int8_t langCode, std::string const & localName)
  {
    bmData.m_featureName[langCode] = localName;
  });
}
