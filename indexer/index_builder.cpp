#include "index_builder.hpp"
#include "feature_processor.hpp"
#include "features_vector.hpp"
#include "../coding/file_reader.hpp"

namespace indexer
{
  bool BuildIndexFromDatFile(string const & fullIndexFilePath, string const & fullDatFilePath,
                             string const & tmpFilePath)
  {
    try
    {
      FileReader dataReader(fullDatFilePath);
      // skip xml header with metadata
      uint64_t startOffset = feature::ReadDatHeaderSize(dataReader);
      FileReader subReader = dataReader.SubReader(startOffset, dataReader.Size() - startOffset);
      FeaturesVector<FileReader> featuresVector(subReader);

      FileWriter indexWriter(fullIndexFilePath.c_str());
      BuildIndex(featuresVector, indexWriter, tmpFilePath);
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
