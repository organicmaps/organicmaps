#include "generator/centers_table_builder.hpp"

#include "search/search_trie.hpp"

#include "indexer/centers_table.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/features_vector.hpp"

#include "coding/file_container.hpp"

#include "platform/mwm_traits.hpp"
#include "platform/platform.hpp"

#include "defines.hpp"

namespace indexer
{
bool BuildCentersTableFromDataFile(string const & filename, bool forceRebuild)
{
  search::CentersTableBuilder builder;

  {
    auto const & platform = GetPlatform();
    FilesContainerR rcont(platform.GetReader(filename, "f"));
    if (!forceRebuild && rcont.IsExist(CENTERS_FILE_TAG))
      return true;

    feature::DataHeader header(rcont);

    version::MwmTraits traits(header.GetFormat());
    if (!traits.HasOffsetsTable())
    {
      LOG(LERROR, (filename, "does not have an offsets table!"));
      return false;
    }
    auto table = feature::FeaturesOffsetsTable::Load(rcont);
    if (!table)
    {
      LOG(LERROR, ("Can't load offsets table from:", filename));
      return false;
    }

    FeaturesVector features(rcont, header, table.get());

    builder.SetCodingParams(header.GetDefCodingParams());
    features.ForEach([&](FeatureType & ft, uint32_t featureId) {
      builder.Put(featureId, feature::GetCenter(ft));
    });
  }

  {
    FilesContainerW writeContainer(filename, FileWriter::OP_WRITE_EXISTING);
    FileWriter writer = writeContainer.GetWriter(CENTERS_FILE_TAG);
    builder.Freeze(writer);
  }

  return true;
}
}  // namespace indexer
