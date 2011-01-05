#include "index_builder.hpp"
#include "data_header_reader.hpp"
#include "features_vector.hpp"

#include "../storage/defines.hpp"

#include "../coding/file_container.hpp"


namespace indexer
{
  bool BuildIndexFromDatFile(string const & datFile, string const & tmpFile)
  {
    try
    {
      FilesContainerR readCont(datFile);
      FeaturesVector featuresVector(readCont);

      FilesContainerW writeCont(datFile, FileWriter::OP_APPEND);
      FileWriter writer = writeCont.GetWriter(INDEX_FILE_TAG);
      BuildIndex(featuresVector, writer, tmpFile);
      writeCont.Finish();
    }
    catch (Reader::OpenException const & e)
    {
      LOG(LERROR, (e.what(), " file is not found"));
      return false;
    }
    catch (Reader::Exception const & e)
    {
      LOG(LERROR, ("Unknown error while reading file ", e.what()));
      return false;
    }
    catch (Writer::Exception const & e)
    {
      LOG(LERROR, ("Error writing index file", e.what()));
    }

    return true;
  }
} // namespace indexer
