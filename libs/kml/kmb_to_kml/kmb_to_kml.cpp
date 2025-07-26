#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "indexer/classificator_loader.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include <iostream>

int main(int argc, char ** argv)
{
  if (argc < 2)
  {
    std::cout << "Converts kmb file(s) to kml\n";
    std::cout << "Usage: " << argv[0] << " path_to_kmb_file [path_to_another_kmb_file...]\n";
    return 1;
  }
  // TODO: Why bookmarks serialization requires classifier?
  classificator::Load();

  do
  {
    std::string filePath = argv[argc - 1];
    kml::FileData kmlData;
    try
    {
      FileReader reader(filePath);
      kml::binary::DeserializerKml des(kmlData);
      des.Deserialize(reader);
    }
    catch (kml::binary::DeserializerKml::DeserializeException const & ex)
    {
      std::cerr << "Error reading kmb file " << filePath << ": " << ex.what() << std::endl;
      return 1;
    }

    try
    {
      // Change extension to kml.
      filePath[filePath.size() - 1] = 'l';
      kml::SerializerKml ser(kmlData);
      FileWriter kmlFile(filePath);
      ser.Serialize(kmlFile);
    }
    catch (kml::SerializerKml::SerializeException const & ex)
    {
      std::cerr << "Error encoding to kml file " << filePath << ": " << ex.what() << std::endl;
      return 1;
    }
    catch (FileWriter::Exception const & ex)
    {
      std::cerr << "Error writing to kml file " << filePath << ": " << ex.what() << std::endl;
      return 1;
    }
    std::cout << "Saved converted file as " << filePath << std::endl;
  }
  while (--argc > 1);
  return 0;
}
