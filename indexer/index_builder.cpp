#include "indexer/index_builder.hpp"

#include "indexer/features_vector.hpp"

#include "base/logging.hpp"

#include "defines.hpp"

namespace indexer
{
bool BuildIndexFromDataFile(std::string const & dataFile, std::string const & tmpFile)
{
  try
  {
    std::string const idxFileName(tmpFile + GEOM_INDEX_TMP_EXT);
    {
      FeaturesVectorTest features(dataFile);
      FileWriter writer(idxFileName);

      BuildIndex(features.GetHeader(), features.GetVector(), writer, tmpFile);
    }

    FilesContainerW(dataFile, FileWriter::OP_WRITE_EXISTING).Write(idxFileName, INDEX_FILE_TAG);
    FileWriter::DeleteFileX(idxFileName);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading file: ", e.Msg()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error writing index file: ", e.Msg()));
    return false;
  }

  return true;
}
}  // namespace indexer
