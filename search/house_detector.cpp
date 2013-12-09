#include "house_detector.hpp"

#include "../indexer/classificator.hpp"

#include "../base/logging.hpp"

#include "../geometry/distance.hpp"
#include "../geometry/distance_on_sphere.hpp"

#include "../std/set.hpp"
#include "../std/bind.hpp"


namespace search
{

string affics1[] =
{
  "аллея", "бульвар", "набережная",
  "переулок", "площадь",  "проезд",
  "проспект", "шоссе", "тупик", "улица"
};

string affics2[] =
{
  "ал.", "бул.", "наб.", "пер.",
  "пл.", "пр.", "просп.", "ш.",
  "туп.", "ул."
};

void GetStreetName(strings::SimpleTokenizer iter, string & streetName)
{
  while(iter)
  {
    bool flag = true;
    for (size_t i = 0; i < ARRAY_SIZE(affics2); ++i)
    {
      if (*iter == affics2[i] || *iter == affics1[i])
      {
        flag = false;
        break;
      }
    }
    if (flag)
      streetName += *iter;
    ++iter;
  }
}

int const BUILDING_PROCESS_SCALE = 15;
double const STREET_CONNECTION_LENGTH_M = 25.0;

void House::InitHouseNumberAndSuffix()
{
  m_suffix = "";
  m_intNumber = 0;
  for (int i = 0; i < m_number.size(); ++i)
    if (m_number[i] < '0' || m_number[i] > '9')
    {
      strings::to_int(m_number.substr(0, i), m_intNumber);
      m_suffix = m_number.substr(i, m_number.size() - i);
      return;
    }
  strings::to_int(m_number, m_intNumber);
}

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

void Street::SetName(string const & name)
{
  m_name = name;
  strings::SimpleTokenizer iter(name, " -");
  GetStreetName(iter, m_processedName);
  strings::MakeLowerCase(m_processedName);
}

namespace
{
bool LessDistance(HouseProjection const & p1, HouseProjection const & p2)
{
  return p1.m_streetDistance < p2.m_streetDistance;
}
}

void Street::SortHousesProjection()
{
  sort(m_houses.begin(), m_houses.end(), &LessDistance);
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
    if (it->second->m_number == -1 && street->GetName() == it->second->GetName())
    {
      it->second->m_number = m_streetNum;
      q.push(it->second);
    }
  }
}

void HouseDetector::Bfs(Street * st)
{
  queue<Street *> q;
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
      st->SetName(name);
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
  vector<m2::PointD> const & m_points;
  double m_distanceMeters;

  typedef m2::ProjectionToSection<m2::PointD> ProjectionT;
  vector<ProjectionT> m_calcs;

public:
  ProjectionCalcToStreet(Street const * st, double distanceMeters)
    : m_points(st->m_points), m_distanceMeters(distanceMeters)
  {
  }

  void Initialize()
  {
    if (m_calcs.empty() && !m_points.empty())
    {
      m_calcs.reserve(m_points.size() - 1);
      for (size_t i = 1; i < m_points.size(); ++i)
      {
        m_calcs.push_back(ProjectionT());
        m_calcs.back().SetBounds(m_points[i-1], m_points[i]);
      }
    }
  }

  void CalculateProjectionParameters(m2::PointD const & pt, m2::PointD & resPt, double & dist, double & resDist, size_t & ind)
  {
    for (size_t i = 0; i < m_calcs.size(); ++i)
    {
      m2::PointD const proj = m_calcs[i](pt);
      dist = GetDistanceMeters(pt, proj);
      if (dist < resDist)
      {
        resPt = proj;
        resDist = dist;
        ind = i;
      }
    }
  }

  bool GetProjection(m2::PointD const & pt, HouseProjection & proj)
  {
    Initialize();

    m2::PointD resPt;
    double dist = numeric_limits<double>::max();
    double resDist = numeric_limits<double>::max();
    size_t ind;

    CalculateProjectionParameters(pt, resPt, dist, resDist, ind);

    if (resDist <= m_distanceMeters)
    {
      proj.m_proj = resPt;
      proj.m_distance = resDist;

      proj.m_streetDistance = 0.0;
      for (size_t i = 0; i < ind; ++i)
        proj.m_streetDistance += m_calcs[i].GetLength();

      proj.m_streetDistance += m_points[ind].Length(proj.m_proj);
      proj.m_projectionSign = m2::GetOrientation(m_points[ind], m_points[ind+1], pt) >= 0;
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
  if (checker.IsHouse(feature::TypesHolder(f)) && !f.GetHouseNumber().empty())
  {
    map<FeatureID, House *>::iterator const it = m_id2house.find(f.GetID());

    m2::PointD const pt = (it == m_id2house.end()) ?
          f.GetLimitRect(BUILDING_PROCESS_SCALE).Center() : it->second->GetPosition();

    HouseProjection pr;
    if (calc.GetProjection(pt, pr))
    {
      House * p;
      if (it == m_id2house.end())
      {
        p = new House(f.GetHouseNumber(), pt);
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

  st->SortHousesProjection();
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

  for (IterM it = m_id2st.begin(); it != m_id2st.end();++it)
  {
    Street const * st = it->second;

    for (size_t i = 0; i < st->m_houses.size(); ++i)
    {
      House const * house = st->m_houses[i].m_house;
      if (house->GetNumber() == houseNumber && s.count(house) == 0)
      {
        res.push_back(st->m_houses[i]);
        s.insert(house);
      }
    }
  }
}



namespace
{
struct LS
{
  size_t prevDecreasePos, decreaseValue;
  size_t prevIncreasePos, increaseValue;

  LS() {}
  LS(size_t i)
  {
    prevDecreasePos = i;
    decreaseValue = 1;
    prevIncreasePos = i;
    increaseValue = 1;
  }
};
}

void LongestSubsequence(vector<HouseProjection> const & houses,
                                       vector<HouseProjection> & result)
{
  if (houses.size() < 2)
  {
    result = houses;
    return;
  }
  vector<LS> v(houses.size());
  for (size_t i = 0; i < v.size(); ++i)
    v[i] = LS(i);

  size_t res = 0;
  size_t pos = 0;
  for (size_t i = 0; i + 1 < houses.size(); ++i)
  {
    for (size_t j = i + 1; j < houses.size(); ++j)
    {
      if (House::LessHouseNumber(*houses[i].m_house, *houses[j].m_house) && v[i].increaseValue + 1 > v[j].increaseValue)
      {
        v[j].increaseValue = v[i].increaseValue + 1;
        v[j].prevIncreasePos = i;
      }
      if (!House::LessHouseNumber(*houses[i].m_house, *houses[j].m_house) && v[i].decreaseValue + 1 > v[j].decreaseValue)
      {
        v[j].decreaseValue = v[i].decreaseValue + 1;
        v[j].prevDecreasePos = i;
      }
      size_t m = max(v[j].increaseValue, v[j].decreaseValue);
      if (m > res)
      {
        res = m;
        pos = j;
      }
    }
  }

  result.resize(res);
  bool increasing = true;
  if (v[pos].increaseValue < v[pos].decreaseValue)
    increasing = false;

  while (res > 0)
  {
    result[res - 1] = houses[pos];
    --res;
    if (increasing)
      pos = v[pos].prevIncreasePos;
    else
      pos = v[pos].prevDecreasePos;
  }
}


pair<HouseDetector::IterM, HouseDetector::IterM> HouseDetector::GetAllStreets()
{
  return pair<HouseDetector::IterM, HouseDetector::IterM> (m_id2st.begin(), m_id2st.end());
}

void AddHouseToMap(search::HouseProjection const & proj, map<search::House, double> & m)
{
  map<search::House, double>::iterator const it = m.find(*proj.m_house);
  if (it != m.end())
  {
    if (it->second > proj.m_distance)
    {
      m.erase(it);
      m.insert(std::pair<search::House, double> (*proj.m_house, proj.m_distance));
    }
  }
  else
    m.insert(std::pair<search::House, double> (*proj.m_house, proj.m_distance));
}

/// @ Simple filter. Delete only houses with same house names. If names are equal take house with smaller distance to projection
void HouseDetector::SimpleFilter(string const & houseNumber, vector<House> & res)
{
  map<search::House, double> result;
  /// @not release version should think about how to return 5 houses for 5 streets.
  for (search::HouseDetector::IterM it = m_id2st.begin(); it != m_id2st.end(); ++it)
  {
    Street * st = it->second;
    for (size_t i = 0; i < st->m_houses.size(); ++i)
      if (st->m_houses[i].m_house->GetNumber() == houseNumber)
        AddHouseToMap(st->m_houses[i], result);
  }
  for (map<search::House, double>::iterator it = result.begin(); it != result.end(); ++it)
    res.push_back(it->first);
}

//filtering house from street

bool cmpH(search::HouseProjection const & h, bool isOdd)
{
  int x = h.m_house->GetIntNumber();
  if ((x % 2 == 1) && isOdd)
    return false;
  if (x % 2 == 0 && !isOdd)
    return false;
  return true;
}

void createKMLString(map<search::House, double> & m)
{
  for (map<search::House, double>::iterator it = m.begin(); it != m.end(); ++it)
  {
    cout << "<Placemark>"
         << "<name>" << it->first.GetNumber() <<  "</name>" <<

         "<Point><coordinates>" <<

            MercatorBounds::XToLon(it->first.GetPosition().x) <<

            "," <<

            MercatorBounds::YToLat(it->first.GetPosition().y) <<

        "</coordinates></Point>" <<
       "</Placemark>" << endl;
  }
}

void ProccessHouses(vector <search::HouseProjection> & houses, vector <search::HouseProjection> & result, bool isOdd, map<search::House, double> & m)
{
  houses.erase(remove_if(houses.begin(), houses.end(), bind(&cmpH, _1, isOdd)), houses.end());
  result.clear();
  LongestSubsequence(houses, result);
  for_each(result.begin(), result.end(), bind(&AddHouseToMap, _1, ref(m)));
}

//valid only if only one street in class
void GetAllHousesForStreet(pair <search::HouseDetector::IterM, search::HouseDetector::IterM> range, map<search::House, double> & m)
{
  for (search::HouseDetector::IterM it = range.first; it != range.second; ++it)
  {
    vector <search::HouseProjection> left, right;
    search::Street * st = it->second;
    double r = numeric_limits<double>::max();
    size_t index = numeric_limits<size_t>::max();
    for (size_t i = 0; i < st->m_houses.size(); ++i)
    {
      double dist = st->m_houses[i].m_distance;
      if (st->m_houses[i].m_projectionSign)
        left.push_back(st->m_houses[i]);
      else
        right.push_back(st->m_houses[i]);
      if (r > dist && st->m_houses[i].m_proj != st->m_points.front() && st->m_houses[i].m_proj != st->m_points.back())
      {
        index = i;
        r = dist;
      }
    }

    if (index >= st->m_houses.size())
    {
      cout << "WARNING!" << endl;
      continue;
    }
    cout << endl;
    bool leftOdd, rightOdd;
    if (!st->m_houses[index].m_projectionSign)
    {
      rightOdd = false;
      if (st->m_houses[index].m_house->GetIntNumber() % 2 == 1)
        rightOdd = true;
      leftOdd = !rightOdd;
    }
    else
    {
      leftOdd = false;
      if (st->m_houses[index].m_house->GetIntNumber() % 2 == 1)
        leftOdd = true;
      rightOdd = !leftOdd;
    }

    vector <search::HouseProjection> leftRes, rightRes;
    ProccessHouses(right, rightRes, rightOdd, m);
    ProccessHouses(left, leftRes, leftOdd, m);
  }
  //createKMLString(m);
}

}
