#include "generator/gen_mwm_info.hpp"

#include <sstream>
#include <tuple>

namespace generator
{
CompositeId::CompositeId(std::string const & str)
{
  std::stringstream stream(str);
  stream.exceptions(std::ios::failbit);
  stream >> m_mainId;
  stream >> m_additionalId;
}

CompositeId::CompositeId(base::GeoObjectId mainId, base::GeoObjectId additionalId)
  : m_mainId(mainId), m_additionalId(additionalId)
{
}

CompositeId::CompositeId(base::GeoObjectId mainId)
  : CompositeId(mainId, base::GeoObjectId() /* additionalId */)
{
}

bool CompositeId::operator<(CompositeId const & other) const
{
  return std::tie(m_mainId, m_additionalId) < std::tie(other.m_mainId, other.m_additionalId);
}

bool CompositeId::operator==(CompositeId const & other) const
{
  return std::tie(m_mainId, m_additionalId) == std::tie(other.m_mainId, other.m_additionalId);
}

bool CompositeId::operator!=(CompositeId const & other) const
{
  return !(*this == other);
}

std::string CompositeId::ToString() const
{
  std::stringstream stream;
  stream.exceptions(std::ios::failbit);
  stream << m_mainId << " " << m_additionalId;
  return stream.str();
}

CompositeId MakeCompositeId(feature::FeatureBuilder const & fb)
{
  CHECK(fb.HasOsmIds(), (fb));
  return CompositeId(fb.GetMostGenericOsmId(), fb.GetFirstOsmId());
}

std::string DebugPrint(CompositeId const & id)
{
  return DebugPrint(id.m_mainId) + "|" + DebugPrint(id.m_additionalId);
}

// static
uint32_t const OsmID2FeatureID::kHeaderMagic;

OsmID2FeatureID::OsmID2FeatureID() : m_version(Version::V1) {}

OsmID2FeatureID::Version OsmID2FeatureID::GetVersion() const { return m_version; }

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
    LOG(LERROR, ("Exception while reading osm id to feature id mapping from file", filename,
                 ". Msg:", e.Msg()));
    return false;
  }

  return true;
}

void OsmID2FeatureID::AddIds(CompositeId const & osmId, uint32_t featureId)
{
  ASSERT(std::find(std::cbegin(m_data), std::cend(m_data), std::make_pair(osmId, featureId)) ==
         std::cend(m_data),
         (osmId));
  m_data.emplace_back(osmId, featureId);
}

boost::optional<uint32_t> OsmID2FeatureID::GetFeatureId(CompositeId const & id) const
{
  auto const it = std::lower_bound(std::cbegin(m_data), std::cend(m_data), id,
                                   [](auto const & l, auto const & r) { return l.first < r; });
  if (it == std::cend(m_data) || it->first != id)
    return {};

  CHECK_NOT_EQUAL(std::next(it)->first, id, (id));
  return it->second;
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

