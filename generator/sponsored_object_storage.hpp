#pragma once

#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"

#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/index/rtree.hpp"

#include "defines.hpp"

namespace generator
{
template <typename Object>
class SponsoredObjectStorage
{
public:
  using ObjectId = typename Object::ObjectId;
  using ObjectsContainer = std::map<ObjectId, Object>;
  using ExcludedIdsContainer = std::unordered_set<ObjectId, typename ObjectId::Hash>;

  SponsoredObjectStorage(double distanceLimitMeters, size_t maxSelectedElements)
    : m_distanceLimitMeters(distanceLimitMeters)
    , m_maxSelectedElements(maxSelectedElements)
  {
  }

  double GetDistanceLimitInMeters() const
  {
    return m_distanceLimitMeters;
  }

  size_t GetMaxSelectedElements() const
  {
    return m_maxSelectedElements;
  }

  ObjectsContainer const & GetObjects() const
  {
    return m_objects;
  }

  size_t Size() const
  {
    return m_objects.size();
  }

  void LoadData(std::string const & dataPath)
  {
    if (dataPath.empty())
      return;

    std::ifstream dataSource(dataPath);
    if (!dataSource)
    {
      LOG(LERROR, ("Error while opening", dataPath, ":", strerror(errno)));
      return;
    }

    auto const excludedIdsPath = my::JoinPath(GetPlatform().ResourcesDir(), BOOKING_EXCLUDED_FILE);

    LoadData(dataSource, LoadExcludedIds(excludedIdsPath));
  }

  ExcludedIdsContainer LoadExcludedIds(std::string const & excludedIdsPath)
  {
    if (excludedIdsPath.empty())
      return {};

    std::ifstream source(excludedIdsPath);
    if (!source)
    {
      LOG(LERROR, ("Error while opening", excludedIdsPath, ":", strerror(errno)));
      return {};
    }

    ExcludedIdsContainer result;
    for (std::string line; std::getline(source, line);)
    {
      ObjectId id{Object::InvalidObjectId()};

      if (!strings::to_any(line, id.Get()))
      {
        LOG(LWARNING, ("Incorrect excluded sponsored id:", line));
        continue;
      }

      if (id != Object::InvalidObjectId())
        result.emplace(id);
    }

    return result;
  }

  void LoadData(std::istream & src, ExcludedIdsContainer const & excludedIds)
  {
    m_objects.clear();
    m_rtree.clear();

    for (std::string line; std::getline(src, line);)
    {
      Object object(line);
      if (object.m_id != Object::InvalidObjectId() &&
          excludedIds.find(object.m_id) == excludedIds.cend())
      {
        m_objects.emplace(object.m_id, object);
      }
    }

    for (auto const & item : m_objects)
    {
      auto const & object = item.second;
      Box b(Point(object.m_latLon.lat, object.m_latLon.lon),
            Point(object.m_latLon.lat, object.m_latLon.lon));
      m_rtree.insert(make_pair(b, object.m_id));
    }
  }

  Object const & GetObjectById(ObjectId id) const
  {
    auto const it = m_objects.find(id);
    CHECK(it != end(m_objects), ("Got wrong object id:", id));
    return it->second;
  }

  Object & GetObjectById(ObjectId id)
  {
    auto const it = m_objects.find(id);
    CHECK(it != end(m_objects), ("Got wrong object id:", id));
    return it->second;
  }

  std::vector<ObjectId> GetNearestObjects(ms::LatLon const & latLon) const
  {
    namespace bgi = boost::geometry::index;

    std::vector<ObjectId> indexes;
    for_each(bgi::qbegin(m_rtree, bgi::nearest(Point(latLon.lat, latLon.lon),
                                               static_cast<unsigned>(m_maxSelectedElements))),
             bgi::qend(m_rtree), [this, &latLon, &indexes](Value const & v) {
               auto const & object = GetObjectById(v.second);
               double const dist = ms::DistanceOnEarth(latLon, object.m_latLon);
               if (m_distanceLimitMeters != 0.0 && dist > m_distanceLimitMeters)
                 return;

               indexes.emplace_back(v.second);
             });

    return indexes;
  }

private:
  // TODO(mgsergio): Get rid of Box since boost::rtree supports point as value type.
  // TODO(mgsergio): Use mercator instead of latlon or boost::geometry::cs::spherical_equatorial
  // instead of boost::geometry::cs::cartesian.
  using Point = boost::geometry::model::point<float, 2, boost::geometry::cs::cartesian>;
  using Box = boost::geometry::model::box<Point>;
  using Value = std::pair<Box, ObjectId>;

  // Create the rtree using default constructor.
  boost::geometry::index::rtree<Value, boost::geometry::index::quadratic<16>> m_rtree;
  ObjectsContainer m_objects;

  double const m_distanceLimitMeters;
  size_t const m_maxSelectedElements;
};
}  // namespace generator
