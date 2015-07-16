#include "indexer/index_builder.hpp"
#include "indexer/features_vector.hpp"

#include "defines.hpp"

#include "base/logging.hpp"


namespace indexer
{
  bool BuildIndexFromDatFile(string const & datFile, string const & tmpFile)
  {
    try
    {
      string const idxFileName(tmpFile + GEOM_INDEX_TMP_EXT);
      {
        FeaturesVectorTest featuresV(datFile);
        FileWriter writer(idxFileName);

        BuildIndex(featuresV.GetHeader(), featuresV.GetVector(), writer, tmpFile);
      }

      FilesContainerW(datFile, FileWriter::OP_WRITE_EXISTING).Write(idxFileName, INDEX_FILE_TAG);
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
} // namespace indexer
