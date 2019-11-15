#include "generator/postcodes_section_builder.hpp"

#include "generator/gen_mwm_info.hpp"

#include "indexer/feature_meta.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/postcodes.hpp"

#include "platform/platform.hpp"

#include "coding/files_container.hpp"
#include "coding/reader.hpp"

#include "base/checked_cast.hpp"

#include <cstdint>
#include <vector>

#include "defines.hpp"

namespace generator
{
bool BuildPostcodesSection(std::string const & mwmFile)
{
  LOG(LINFO, ("Building the Postcodes section"));

  FilesContainerR container(GetPlatform().GetReader(mwmFile, "f"));
  ReaderSource<ModelReaderPtr> src(container.GetReader(TEMP_ADDR_FILE_TAG));
  std::vector<feature::AddressData> addrs;
  while (src.Size() > 0)
  {
    addrs.push_back({});
    addrs.back().Deserialize(src);
  }

  auto const featuresCount = base::checked_cast<uint32_t>(addrs.size());

  indexer::PostcodesBuilder builder;
  bool havePostcodes = false;
  feature::ForEachFromDat(mwmFile, [&](FeatureType const & f, uint32_t featureId) {
    CHECK_LESS(featureId, featuresCount, ());
    auto const postcode = addrs[featureId].Get(feature::AddressData::Type::Postcode);
    if (postcode.empty())
      return;

    havePostcodes = true;
    builder.Put(featureId, postcode);
  });

  if (!havePostcodes)
    return true;

  FilesContainerW writeContainer(mwmFile, FileWriter::OP_WRITE_EXISTING);
  auto writer = writeContainer.GetWriter(POSTCODES_FILE_TAG);
  builder.Freeze(*writer);
  return true;
}
}  // namespace generator
