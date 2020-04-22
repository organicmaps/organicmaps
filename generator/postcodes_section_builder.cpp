#include "generator/postcodes_section_builder.hpp"

#include "generator/gen_mwm_info.hpp"

#include "indexer/feature_algo.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/postcodes.hpp"

#include "platform/platform.hpp"

#include "coding/files_container.hpp"
#include "coding/reader.hpp"

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include "base/checked_cast.hpp"
#include "base/file_name_utils.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include "defines.hpp"

namespace generator
{
bool BuildPostcodesSection(std::string const & path, std::string const & country,
                           std::string const & boundaryPostcodesFilename)
{
  LOG(LINFO, ("Building the Postcodes section"));

  std::vector<feature::AddressData> addrs;
  {
    auto const addrFile = base::JoinPath(path, country + DATA_FILE_EXTENSION + TEMP_ADDR_FILENAME);
    FileReader reader(addrFile);
    ReaderSource<FileReader> src(reader);
    while (src.Size() > 0)
    {
      addrs.push_back({});
      addrs.back().Deserialize(src);
    }
  }

  auto const featuresCount = base::checked_cast<uint32_t>(addrs.size());

  std::vector<std::pair<std::string, m2::RegionD>> boundaryPostcodes;
  m4::Tree<size_t> boundariesTree;

  // May be empty for tests because TestMwmBuilder cannot collect data from osm elements.
  if (!boundaryPostcodesFilename.empty())
  {
    FileReader reader(boundaryPostcodesFilename);
    ReaderSource<FileReader> src(reader);

    while (src.Size() > 0)
    {
      std::string postcode;
      utils::ReadString(src, postcode);
      std::vector<m2::PointD> geometry;
      rw::ReadVectorOfPOD(src, geometry);
      boundaryPostcodes.emplace_back(std::move(postcode), std::move(geometry));
      boundariesTree.Add(boundaryPostcodes.size() - 1, boundaryPostcodes.back().second.GetRect());
    }
  }

  indexer::PostcodesBuilder builder;
  size_t postcodesFromAddr = 0;
  size_t postcodesFromBoundaries = 0;
  auto const mwmFile = base::JoinPath(path, country + DATA_FILE_EXTENSION);
  feature::ForEachFeature(mwmFile, [&](FeatureType & f, uint32_t featureId) {
    CHECK_LESS(featureId, featuresCount, ());
    auto const postcode = addrs[featureId].Get(feature::AddressData::Type::Postcode);
    if (!postcode.empty())
    {
      ++postcodesFromAddr;
      builder.Put(featureId, postcode);
      return;
    }

    if (!ftypes::IsAddressObjectChecker::Instance()(f))
      return;

    std::string name;
    f.GetReadableName(name);

    auto const houseNumber = f.GetHouseNumber();

    // We do not save postcodes for unnamed features without house number to reduce amount of data.
    // For example with this filter we have +100Kb for Turkey_Marmara Region_Istanbul.mwm, without
    // filter we have +3Mb because of huge amount of unnamed houses without number.
    if (name.empty() && houseNumber.empty())
      return;

    auto const center = feature::GetCenter(f);
    boundariesTree.ForAnyInRect(m2::RectD(center, center), [&](size_t i) {
      CHECK_LESS(i, boundaryPostcodes.size(), ());
      if (!boundaryPostcodes[i].second.Contains(center))
        return false;

      ++postcodesFromBoundaries;
      builder.Put(featureId, boundaryPostcodes[i].first);
      return true;
    });
  });

  if (postcodesFromBoundaries == 0 && postcodesFromAddr == 0)
    return true;

  LOG(LINFO, ("Adding", postcodesFromAddr, "postcodes from addr:postalcode and",
              postcodesFromBoundaries, "postcodes from boundary=postal_code relation."));

  FilesContainerW writeContainer(mwmFile, FileWriter::OP_WRITE_EXISTING);
  auto writer = writeContainer.GetWriter(POSTCODES_FILE_TAG);
  builder.Freeze(*writer);
  return true;
}
}  // namespace generator
