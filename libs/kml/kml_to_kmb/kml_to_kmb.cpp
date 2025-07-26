#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"
#include "kml/serdes_binary_v8.hpp"

#include "indexer/classificator_loader.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include <iostream>

int main(int argc, char ** argv)
{
  if (argc < 2)
  {
    std::cout << "Converts kml file to kmb\n";
    std::cout << "Usage: " << argv[0] << " path_to_kml_file [KBM_FORMAT_VERSION]\n";
    std::cout << "KBM_FORMAT_VERSION could be V8, V9, or Latest (default)\n";
    return 1;
  }
  kml::binary::Version outVersion = kml::binary::Version::Latest;
  if (argc >= 3)
  {
    std::string const versionStr = argv[2];
    if (versionStr == "V8")
      outVersion = kml::binary::Version::V8;
    else if (versionStr != "V9" && versionStr != "Latest")
    {
      std::cout << "Invalid format version: " << versionStr << '\n';
      return 2;
    }
  }
  // TODO: Why bookmarks serialization requires classifier?
  classificator::Load();

  std::string filePath = argv[1];
  kml::FileData kmlData;
  try
  {
    FileReader reader(filePath);
    kml::DeserializerKml des(kmlData);
    des.Deserialize(reader);
  }
  catch (kml::DeserializerKml::DeserializeException const & ex)
  {
    std::cerr << "Error reading kml file " << filePath << ": " << ex.what() << std::endl;
    return 1;
  }

  try
  {
    // Change extension to kmb.
    filePath[filePath.size() - 1] = 'b';
    if (outVersion == kml::binary::Version::V9)
    {
      kml::binary::SerializerKml ser(kmlData);
      FileWriter kmlFile(filePath);
      ser.Serialize(kmlFile);
    }
    else if (outVersion == kml::binary::Version::V8)
    {
      kml::binary::SerializerKmlV8 ser(kmlData);
      FileWriter kmlFile(filePath);
      ser.Serialize(kmlFile);
    }
  }
  catch (kml::SerializerKml::SerializeException const & ex)
  {
    std::cerr << "Error encoding to kmb file " << filePath << ": " << ex.what() << std::endl;
    return 1;
  }
  catch (FileWriter::Exception const & ex)
  {
    std::cerr << "Error writing to kmb file " << filePath << ": " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
