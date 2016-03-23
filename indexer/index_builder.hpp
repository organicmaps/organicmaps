#pragma once

#include "indexer/data_header.hpp"
#include "indexer/scale_index_builder.hpp"

namespace indexer
{
template <class TFeaturesVector, typename TWriter>
void BuildIndex(feature::DataHeader const & header, TFeaturesVector const & features,
                TWriter & writer, string const & tmpFilePrefix)
  {
    LOG(LINFO, ("Building scale index."));
    uint64_t indexSize;
    {
      SubWriter<TWriter> subWriter(writer);
      covering::IndexScales(header, features, subWriter, tmpFilePrefix);
      indexSize = subWriter.Size();
    }
    LOG(LINFO, ("Built scale index. Size =", indexSize));
  }

  // doesn't throw exceptions
  bool BuildIndexFromDataFile(string const & datFile, string const & tmpFile);
}
