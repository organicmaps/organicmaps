#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "indexer/classificator_loader.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include <iostream>


int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cout << "Converts kml file to kmb\n";
    std::cout << "Usage: " << argv[0] << " path_to_kml_file [KMB_VERSION]\n";
    std::cout << "    KMB_VERSION could be: ";
    for(int i=0; i <= static_cast<int>(kml::binary::Version::Latest); i++)
    {
      if (i != 0)
        std::cout << ", ";
      std::cout << VersionToName(static_cast<kml::binary::Version>(i));
    }
    return 1;
  }

  //
  // TODO: Why bookmarks serialization requires classifier?
  classificator::Load();

  std::string filePath = argv[1];
  kml::binary::Version ver = kml::binary::Version::Latest;
  if (argc == 3)
  {
    std::optional<kml::binary::Version> maybeVer = kml::binary::NameToVersion(argv[2]);
    if (!maybeVer.has_value())
    {
      std::cout << "Invalid KMB_VERSION: " << argv[2] << '\n';
      return 2;
    }
    ver = maybeVer.value();
  }

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
    kml::binary::SerializerKml ser(kmlData);
    FileWriter kmlFile(filePath);
    ser.Serialize(kmlFile, ver);
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
