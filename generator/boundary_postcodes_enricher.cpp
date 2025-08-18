#include "generator/boundary_postcodes_enricher.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "platform/platform.hpp"

#include "coding/read_write_utils.hpp"

#include "geometry/point2d.hpp"

namespace generator
{
BoundaryPostcodesEnricher::BoundaryPostcodesEnricher(std::string const & boundaryPostcodesFilename)
{
  // May be absent for tests because TestMwmBuilder cannot collect data from osm elements.
  if (!Platform::IsFileExistsByFullPath(boundaryPostcodesFilename))
    return;

  FileReader reader(boundaryPostcodesFilename);
  ReaderSource<FileReader> src(reader);

  while (src.Size() > 0)
  {
    std::string postcode;
    rw::ReadNonEmpty(src, postcode);
    std::vector<m2::PointD> geometry;
    rw::ReadVectorOfPOD(src, geometry);
    CHECK(!postcode.empty() && !geometry.empty(), ());

    m_boundaryPostcodes.emplace_back(std::move(postcode), std::move(geometry));
    m_boundariesTree.Add(m_boundaryPostcodes.size() - 1, m_boundaryPostcodes.back().second.GetRect());
  }
}

void BoundaryPostcodesEnricher::Enrich(feature::FeatureBuilder & fb) const
{
  auto & params = fb.GetParams();
  if (!params.GetPostcode().empty() || !ftypes::IsAddressObjectChecker::Instance()(fb.GetTypes()))
    return;

  auto const hasName = !fb.GetMultilangName().IsEmpty();
  auto const hasHouseNumber = !params.house.IsEmpty();

  // We do not save postcodes for unnamed features without house number to reduce amount of data.
  // For example with this filter we have +100Kb for Turkey_Marmara Region_Istanbul.mwm, without
  // filter we have +3Mb because of huge amount of unnamed houses without number.
  if (!hasName && !hasHouseNumber)
    return;

  auto const center = fb.GetKeyPoint();
  m_boundariesTree.ForAnyInRect(m2::RectD(center, center), [&](size_t i)
  {
    CHECK_LESS(i, m_boundaryPostcodes.size(), ());
    if (!m_boundaryPostcodes[i].second.Contains(center))
      return false;

    params.SetPostcode(m_boundaryPostcodes[i].first);
    return true;
  });
}
}  // namespace generator
