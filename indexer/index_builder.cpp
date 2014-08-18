#include "index_builder.hpp"
#include "features_vector.hpp"

#include "../defines.hpp"

#include "../base/logging.hpp"


namespace indexer
{
  bool BuildIndexFromDatFile(string const & datFile, string const & tmpFile)
  {
    try
    {
      // First - open container for writing (it can be reallocated).
      FilesContainerW writeCont(datFile, FileWriter::OP_WRITE_EXISTING);
      FileWriter writer = writeCont.GetWriter(INDEX_FILE_TAG);

      // Second - open container for reading.
      FilesContainerR readCont(datFile);

      feature::DataHeader header;
      header.Load(readCont.GetReader(HEADER_FILE_TAG));

      FeaturesVector featuresVector(readCont, header);

      BuildIndex(header.GetLastScale() + 1, header.GetLastScale(), featuresVector, writer, tmpFile);
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
