#include "generator/gen_mwm_info.hpp"

namespace generator
{
CompositeId MakeCompositeId(feature::FeatureBuilder const & fb)
{
  CHECK(fb.HasOsmIds(), (fb));
  return CompositeId(fb.GetMostGenericOsmId(), fb.GetFirstOsmId());
}

// static
uint32_t const OsmID2FeatureID::kHeaderMagic;

OsmID2FeatureID::OsmID2FeatureID() : m_version(Version::V1) {}

OsmID2FeatureID::Version OsmID2FeatureID::GetVersion() const
{
  return m_version;
}

bool OsmID2FeatureID::ReadFromFile(std::string const & filename)
{
  try
  {
    FileReader reader(filename);
    NonOwningReaderSource src(reader);
    ReadAndCheckHeader(src);
  }
  catch (FileReader::Exception const & e)
  {
    LOG(LERROR, ("Exception while reading osm id to feature id mapping from file", filename, ". Msg:", e.Msg()));
    return false;
  }

  return true;
}

void OsmID2FeatureID::AddIds(CompositeId const & osmId, uint32_t featureId)
{
  ASSERT(!base::IsExist(m_data, std::make_pair(osmId, featureId)), (osmId));
  m_data.emplace_back(osmId, featureId);
}

std::vector<uint32_t> OsmID2FeatureID::GetFeatureIds(CompositeId const & id) const
{
  std::vector<uint32_t> ids;
  auto it = std::lower_bound(std::cbegin(m_data), std::cend(m_data), id,
                             [](auto const & l, auto const & r) { return l.first < r; });
  while (it != std::cend(m_data) && it->first == id)
    ids.emplace_back((it++)->second);

  return ids;
}

std::vector<uint32_t> OsmID2FeatureID::GetFeatureIds(base::GeoObjectId mainId) const
{
  std::vector<uint32_t> ids;
  auto it = std::lower_bound(std::cbegin(m_data), std::cend(m_data), mainId,
                             [](auto const & l, auto const & r) { return l.first.m_mainId < r; });
  while (it != std::cend(m_data) && it->first.m_mainId == mainId)
    ids.emplace_back((it++)->second);
  return ids;
}
}  // namespace generator
