#include "indexer/locality_index.hpp"

#include "coding/file_container.hpp"
#include "coding/mmap_reader.hpp"

#include "defines.hpp"

namespace indexer
{
std::string ReadGeneratorDataVersionFromIndex(std::string const & filename)
{
  FilesContainerR indexContainer(filename);
  std::pair<uint64_t, uint64_t> offsetsAndSize =
      indexContainer.GetAbsoluteOffsetAndSize(INDEX_GENERATOR_DATA_VERSION_FILE_TAG);
  MmapReader fileReader(filename);
  ReaderPtr<Reader> reader(fileReader.CreateSubReader(offsetsAndSize.first, offsetsAndSize.second));

  std::string result;
  reader.ReadAsString(result);
  return result;
}
}  // namespace indexer
