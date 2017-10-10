#include "generator/ugc_section_builder.hpp"

#include "generator/gen_mwm_info.hpp"
#include "generator/ugc_translator.hpp"

#include "ugc/binary/index_ugc.hpp"
#include "ugc/binary/serdes.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/ftraits.hpp"

#include <unordered_map>
#include <utility>

namespace generator
{
bool BuildUgcMwmSection(std::string const & srcDbFilename, std::string const & mwmFile,
                        std::string const & osmToFeatureFilename)
{
  using ugc::binary::IndexUGC;

  LOG(LINFO, ("Build UGC section"));

  gen::OsmID2FeatureID osmIdsToFeatureIds;
  if (!osmIdsToFeatureIds.ReadFromFile(osmToFeatureFilename))
    return false;

  std::unordered_map<uint32_t, osm::Id> featureToOsmId;
  osmIdsToFeatureIds.ForEach([&featureToOsmId](gen::OsmID2FeatureID::ValueT const & p) {
    featureToOsmId.emplace(p.second /* feature id */, p.first /* osm id */);
  });

  UGCTranslator translator(srcDbFilename);

  std::vector<IndexUGC> content;

  feature::ForEachFromDat(mwmFile, [&](FeatureType const & f, uint32_t featureId) {
    auto const ugcMasks = ftraits::UGC::GetValue({f});

    if (!ftraits::UGC::IsUGCAvailable(ugcMasks))
      return;

    auto const it = featureToOsmId.find(featureId);
    CHECK(it != featureToOsmId.cend(),
          ("FeatureID", featureId, "is not found in", osmToFeatureFilename));

    ugc::UGC result;
    if (!translator.TranslateUGC(it->second, result))
      return;

    content.emplace_back(featureId, result);
  });

  if (content.empty())
    return true;

  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(UGC_FILE_TAG);
  ugc::binary::UGCSeriaizer serializer(std::move(content));
  serializer.Serialize(writer);

  return true;
}
}  // namespace generator
