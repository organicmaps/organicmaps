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
    std::cout << "Usage: " << argv[0] << " path_to_kml_file\n";
    return 1;
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
    kml::binary::SerializerKml ser(kmlData);
    FileWriter kmlFile(filePath);
    ser.Serialize(kmlFile);
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
