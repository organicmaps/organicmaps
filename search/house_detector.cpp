#include "house_detector.hpp"

#include "../indexer/classificator.hpp"

#include "../base/logging.hpp"

#include "../geometry/distance.hpp"
#include "../geometry/distance_on_sphere.hpp"

#include "../std/iostream.hpp"
#include "../std/set.hpp"
#include "../std/bind.hpp"


namespace search
{

int const BUILDING_PROCESS_SCALE = 15;
double const STREET_CONNECTION_LENGTH_M = 25.0;


struct StreetCreator
{
  Street * street;
  StreetCreator(Street * st): street(st)
  {}
  void operator () (CoordPointT const & point)
  {
    street->m_points.push_back(m2::PointD(point.first, point.second));
  }
};

FeatureLoader::FeatureLoader(Index const * pIndex)
  : m_pIndex(pIndex), m_pGuard(0)
{
}

FeatureLoader::~FeatureLoader()
{
  Free();
}

void FeatureLoader::CreateLoader(size_t mwmID)
{
  if (m_pGuard == 0 || mwmID != m_pGuard->GetID())
  {
    delete m_pGuard;
    m_pGuard = new Index::FeaturesLoaderGuard(*m_pIndex, mwmID);
  }
}

void FeatureLoader::Load(FeatureID const & id, FeatureType & f)
{
  CreateLoader(id.m_mwm);
  m_pGuard->GetFeature(id.m_offset, f);
}

void FeatureLoader::Free()
{
  delete m_pGuard;
  m_pGuard = 0;
}

template <class ToDo>
void FeatureLoader::ForEachInRect(m2::RectD const & rect, ToDo toDo)
{
  m_pIndex->ForEachInRect(toDo, rect, BUILDING_PROCESS_SCALE);
}

m2::RectD Street::GetLimitRect(double offsetMeters) const
{
  m2::RectD rect;
  for (size_t i = 0; i < m_points.size(); ++i)
    rect.Add(MercatorBounds::RectByCenterXYAndSizeInMeters(m_points[i], offsetMeters));
  return rect;
}


HouseDetector::HouseDetector(Index const * pIndex)
  : m_loader(pIndex), m_end2st(LessWithEpsilon(&m_epsMercator)), m_streetNum(0)
{
  // default value for conversions
  SetMetres2Mercator(360.0 / 40.0E06);
}

void HouseDetector::SetMetres2Mercator(double factor)
{
  m_epsMercator = factor * STREET_CONNECTION_LENGTH_M;
}

void HouseDetector::FillQueue(queue<Street *> & q, Street const * street, bool isBeg)
{
  pair<IterT, IterT> const & range =
      m_end2st.equal_range(isBeg ? street->m_points.front() : street->m_points.back());

  for (IterT it = range.first; it != range.second; ++it)
  {
    /// @todo create clever street names compare functions
    if (it->second->m_number == -1 && street->m_name == it->second->m_name)
    {
      it->second->m_number = m_streetNum;
      q.push(it->second);
    }
  }
}

void HouseDetector::Bfs(Street * st)
{
  queue <Street *> q;
  q.push(st);
  st->m_number = m_streetNum;
  while(!q.empty())
  {
    Street * street = q.front();
    q.pop();
    FillQueue(q, street, true);
    FillQueue(q, street, false);
  }
}

void HouseDetector::LoadStreets(vector<FeatureID> & ids)
{
  sort(ids.begin(), ids.end());

  for (size_t i = 0; i < ids.size(); ++i)
  {
    if (m_id2st.find(ids[i]) != m_id2st.end())
      continue;

    FeatureType f;
    m_loader.Load(ids[i], f);
    if (f.GetFeatureType() == feature::GEOM_LINE)
    {
      /// @todo Assume that default name always exist as primary compare key.
      string name;
      if (!f.GetName(0, name) || name.empty())
        continue;

      Street * st = new Street();
      st->m_name = name;
      f.ForEachPoint(StreetCreator(st), FeatureType::BEST_GEOMETRY);

      m_id2st[ids[i]] = st;
      m_end2st.insert(pair<m2::PointD, Street *> (st->m_points.front(), st));
      m_end2st.insert(pair<m2::PointD, Street *> (st->m_points.back(), st));
    }
    else
      ASSERT(false, ());
  }

  m_loader.Free();
}

int HouseDetector::MergeStreets()
{
  for (IterT it = m_end2st.begin(); it != m_end2st.end(); ++it)
  {
    if (it->second->m_number == -1)
    {
      Street * st = it->second;
      ++m_streetNum;
      Bfs(st);
    }
  }
  return m_streetNum;
}

namespace
{

class HouseChecker
{
  uint32_t m_types[2];
public:
  HouseChecker()
  {
    Classificator const & c = classif();

    char const * arr0[] = { "building" };
    m_types[0] = c.GetTypeByPath(vector<string>(arr0, arr0 + 1));
    char const * arr1[] = { "building", "address" };
    m_types[1] = c.GetTypeByPath(vector<string>(arr1, arr1 + 2));
  }

  bool IsHouse(feature::TypesHolder const & types)
  {
    for (size_t i = 0; i < ARRAY_SIZE(m_types); ++i)
      if (types.Has(m_types[i]))
        return true;
    return false;
  }
};

double GetDistanceMeters(m2::PointD const & p1, m2::PointD const & p2)
{
  return ms::DistanceOnEarth(MercatorBounds::YToLat(p1.y), MercatorBounds::XToLon(p1.x),
                             MercatorBounds::YToLat(p2.y), MercatorBounds::XToLon(p2.x));
}

class ProjectionCalcToStreet
{
  Street const * m_street;
  double m_distanceMeters;

  typedef m2::ProjectionToSection<m2::PointD> ProjectionT;
  vector<ProjectionT> m_calcs;

public:
  ProjectionCalcToStreet(Street const * st, double distanceMeters)
    : m_street(st), m_distanceMeters(distanceMeters)
  {
  }

  bool GetProjection(m2::PointD const & pt, HouseProjection & proj)
  {
    if (m_calcs.empty())
    {
      for (size_t i = 1; i < m_street->m_points.size(); ++i)
      {
        m_calcs.push_back(ProjectionT());
        m_calcs.back().SetBounds(m_street->m_points[i-1], m_street->m_points[i]);
      }
    }

    m2::PointD resPt;
    double dist = numeric_limits<double>::max();
    double resDist = numeric_limits<double>::max();
    for (size_t i = 0; i < m_calcs.size(); ++i)
    {
      m2::PointD const proj = m_calcs[i](pt);
      dist = GetDistanceMeters(pt, proj);
      if (dist < resDist)
      {
        resPt = proj;
        resDist = dist;
      }
    }

    if (resDist <= m_distanceMeters)
    {
      proj.m_proj = resPt;
      proj.m_distance = dist;
      return true;
    }
    else
      return false;
  }
};

}

template <class ProjectionCalcT>
void HouseDetector::ReadHouse(FeatureType const & f, Street * st, ProjectionCalcT & calc)
{
  static HouseChecker checker;
  if (checker.IsHouse(feature::TypesHolder(f)))
  {
    map<FeatureID, House *>::iterator const it = m_id2house.find(f.GetID());

    m2::PointD const pt = (it == m_id2house.end()) ?
          f.GetLimitRect(BUILDING_PROCESS_SCALE).Center() : it->second->m_point;

    HouseProjection pr;
    if (calc.GetProjection(pt, pr))
    {
      House * p;
      if (it == m_id2house.end())
      {
        p = new House();
        p->m_point = pt;
        p->m_number = f.GetHouseNumber();
        m_id2house[f.GetID()] = p;
      }
      else
      {
        p = it->second;
        ASSERT(p != 0, ());
      }

      pr.m_house = p;
      st->m_houses.push_back(pr);
    }
  }
}

void HouseDetector::ReadHouses(Street * st, double offsetMeters)
{
  if (st->m_housesReaded)
    return;

  ProjectionCalcToStreet calcker(st, offsetMeters);
  m_loader.ForEachInRect(st->GetLimitRect(offsetMeters),
                         bind(&HouseDetector::ReadHouse<ProjectionCalcToStreet>, this, _1, st, ref(calcker)));

  st->m_housesReaded = true;
}

void HouseDetector::ReadAllHouses(double offsetMeters)
{
  for (map<FeatureID, Street *>::iterator it = m_id2st.begin(); it != m_id2st.end(); ++it)
    ReadHouses(it->second, offsetMeters);
}

void HouseDetector::MatchAllHouses(string const & houseNumber, vector<HouseProjection> & res)
{
  /// @temporary decision to avoid duplicating houses
  set<House const *> s;

  for (map<FeatureID, Street *>::iterator it = m_id2st.begin(); it != m_id2st.end();++it)
  {
    Street const * st = it->second;
    for (size_t i = 0; i < st->m_houses.size(); ++i)
    {
      House const * house = st->m_houses[i].m_house;
      if (house->m_number == houseNumber && s.count(house) == 0)
      {
        res.push_back(st->m_houses[i]);
        s.insert(house);
      }
    }
  }
}

}
