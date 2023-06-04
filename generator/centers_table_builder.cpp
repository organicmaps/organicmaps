#include "generator/centers_table_builder.hpp"

#include "indexer/centers_table.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/features_vector.hpp"

#include "coding/files_container.hpp"

#include "base/exception.hpp"

#include "defines.hpp"

namespace indexer
{
bool BuildCentersTableFromDataFile(std::string const & filename, bool forceRebuild)
{
  try
  {
    search::CentersTableBuilder builder;

    {
      FilesContainerR rcont(filename);
      if (!forceRebuild && rcont.IsExist(CENTERS_FILE_TAG))
        return true;

      auto const table = feature::FeaturesOffsetsTable::Load(rcont);
      if (!table)
      {
        LOG(LERROR, ("Can't load offsets table from:", filename));
        return false;
      }

      feature::DataHeader const header(rcont);
      FeaturesVector const features(rcont, header, table.get(), nullptr);

      builder.SetGeometryParams(header.GetBounds());
      features.ForEach([&](FeatureType & ft, uint32_t featureId)
      {
        builder.Put(featureId, feature::GetCenter(ft));
      });
    }

    {
      FilesContainerW writeContainer(filename, FileWriter::OP_WRITE_EXISTING);
      auto writer = writeContainer.GetWriter(CENTERS_FILE_TAG);
      builder.Freeze(*writer);
    }
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Failed to build centers table:", e.Msg()));
    return false;
  }

  return true;
}
}  // namespace indexer
