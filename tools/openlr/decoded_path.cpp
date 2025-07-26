#include "openlr/decoded_path.hpp"

#include "indexer/data_source.hpp"

#include "platform/country_file.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <sstream>

#define THROW_IF_NODE_IS_EMPTY(node, exc, msg) \
  if (!node)                                   \
  MYTHROW(exc, msg)

namespace
{
uint32_t UintFromXML(pugi::xml_node const & node)
{
  THROW_IF_NODE_IS_EMPTY(node, openlr::DecodedPathLoadError, ("Can't parse uint"));
  return node.text().as_uint();
}

bool IsForwardFromXML(pugi::xml_node const & node)
{
  THROW_IF_NODE_IS_EMPTY(node, openlr::DecodedPathLoadError, ("Can't parse IsForward"));
  return node.text().as_bool();
}

void LatLonToXML(ms::LatLon const & latLon, pugi::xml_node & node)
{
  auto const kDigitsAfterComma = 5;
  node.append_child("lat").text() = strings::to_string_dac(latLon.m_lat, kDigitsAfterComma).data();
  node.append_child("lon").text() = strings::to_string_dac(latLon.m_lon, kDigitsAfterComma).data();
}

void LatLonFromXML(pugi::xml_node const & node, ms::LatLon & latLon)
{
  THROW_IF_NODE_IS_EMPTY(node, openlr::DecodedPathLoadError, ("Can't parse latLon"));
  latLon.m_lat = node.child("lat").text().as_double();
  latLon.m_lon = node.child("lon").text().as_double();
}

void FeatureIdFromXML(pugi::xml_node const & node, DataSource const & dataSource, FeatureID & fid)
{
  THROW_IF_NODE_IS_EMPTY(node, openlr::DecodedPathLoadError, ("Can't parse CountryName"));
  auto const countryName = node.child("CountryName").text().as_string();
  fid.m_mwmId = dataSource.GetMwmIdByCountryFile(platform::CountryFile(countryName));
  CHECK(fid.m_mwmId.IsAlive(), ("Can't get mwm id for country", countryName));
  fid.m_index = node.child("Index").text().as_uint();
}

void FeatureIdToXML(FeatureID const & fid, pugi::xml_node & node)
{
  node.append_child("CountryName").text() = fid.m_mwmId.GetInfo()->GetCountryName().data();
  node.append_child("Index").text() = fid.m_index;
}
}  // namespace

namespace openlr
{
uint32_t UintValueFromXML(pugi::xml_node const & node)
{
  auto const value = node.child("olr:value");
  if (!value)
    return 0;

  return UintFromXML(value);
}

void WriteAsMappingForSpark(std::string const & fileName, std::vector<DecodedPath> const & paths)
{
  std::ofstream ofs(fileName);
  if (!ofs.is_open())
    MYTHROW(DecodedPathSaveError, ("Can't write to file", fileName, strerror(errno)));

  WriteAsMappingForSpark(ofs, paths);

  if (ofs.fail())
    MYTHROW(DecodedPathSaveError, ("An error occured while writing file", fileName, strerror(errno)));
}

void WriteAsMappingForSpark(std::ostream & ost, std::vector<DecodedPath> const & paths)
{
  auto const flags = ost.flags();
  ost << std::fixed;  // Avoid scientific notation cause '-' is used as fields separator.
  SCOPE_GUARD(guard, ([&ost, &flags] { ost.flags(flags); }));

  for (auto const & p : paths)
  {
    if (p.m_path.empty())
      continue;

    ost << p.m_segmentId.Get() << '\t';

    auto const kFieldSep = '-';
    auto const kSegmentSep = '=';
    for (auto it = std::begin(p.m_path); it != std::end(p.m_path); ++it)
    {
      auto const & fid = it->GetFeatureId();
      ost << fid.m_mwmId.GetInfo()->GetCountryName() << kFieldSep << fid.m_index << kFieldSep << it->GetSegId()
          << kFieldSep << (it->IsForward() ? "fwd" : "bwd") << kFieldSep
          << mercator::DistanceOnEarth(GetStart(*it), GetEnd(*it));

      if (std::next(it) != std::end(p.m_path))
        ost << kSegmentSep;
    }
    ost << std::endl;
  }
}

void PathFromXML(pugi::xml_node const & node, DataSource const & dataSource, Path & p)
{
  auto const edges = node.select_nodes("RoadEdge");
  for (auto const & xmlE : edges)
  {
    auto e = xmlE.node();

    FeatureID fid;
    FeatureIdFromXML(e.child("FeatureID"), dataSource, fid);

    auto const isForward = IsForwardFromXML(e.child("IsForward"));
    auto const segmentId = UintFromXML(e.child("SegmentId"));

    ms::LatLon start, end;
    LatLonFromXML(e.child("StartJunction"), start);
    LatLonFromXML(e.child("EndJunction"), end);

    p.push_back(
        Edge::MakeReal(fid, isForward, segmentId,
                       geometry::PointWithAltitude(mercator::FromLatLon(start), geometry::kDefaultAltitudeMeters),
                       geometry::PointWithAltitude(mercator::FromLatLon(end), geometry::kDefaultAltitudeMeters)));
  }
}

void PathToXML(Path const & path, pugi::xml_node & node)
{
  for (auto const & e : path)
  {
    auto edge = node.append_child("RoadEdge");

    {
      auto fid = edge.append_child("FeatureID");
      FeatureIdToXML(e.GetFeatureId(), fid);
    }

    edge.append_child("IsForward").text() = e.IsForward();
    edge.append_child("SegmentId").text() = e.GetSegId();
    {
      auto start = edge.append_child("StartJunction");
      auto end = edge.append_child("EndJunction");
      LatLonToXML(mercator::ToLatLon(GetStart(e)), start);
      LatLonToXML(mercator::ToLatLon(GetEnd(e)), end);
    }
  }
}
}  // namespace openlr

namespace routing
{
std::vector<m2::PointD> GetPoints(openlr::Path const & p)
{
  CHECK(!p.empty(), ("Path should not be empty"));
  std::vector<m2::PointD> points;
  points.push_back(GetStart(p.front()));
  for (auto const & e : p)
    points.push_back(GetEnd(e));

  return points;
}
}  // namespace routing
