#pragma once

#include "../indexer/feature_decl.hpp"
#include "../indexer/index.hpp"

#include "../geometry/point2d.hpp"

#include "../std/string.hpp"
#include "../std/queue.hpp"


namespace search
{
class FeatureLoader
{
  Index const * m_pIndex;
  Index::FeaturesLoaderGuard * m_pGuard;

  void CreateLoader(size_t mwmID);

public:
  FeatureLoader(Index const * pIndex);
  ~FeatureLoader();

  void Load(FeatureID const & id, FeatureType & f);
  void Free();

  template <class ToDo> void ForEachInRect(m2::RectD const & rect, ToDo toDo);
};

class House
{
  string m_number;
  m2::PointD m_point;

  // Start and End house numbers for this object.
  // Osm objects can store many numbers for one area feature.
  int m_startN, m_endN;

  void InitHouseNumber();

public:
  House(string const & number, m2::PointD const & point)
    : m_number(number), m_point(point), m_startN(-1), m_endN(-1)
  {
    InitHouseNumber();
  }

  inline string const & GetNumber() const { return m_number; }
  inline int GetIntNumber() const { return m_startN; }
  inline m2::PointD const & GetPosition() const { return m_point; }

  struct LessHouseNumber
  {
    bool operator() (House const * h1, House const * h2) const
    {
      return (h1->m_startN < h2->m_startN);
    }
  };

  struct ParsedNumber
  {
    string const * m_fullN;
    int m_intN;

    ParsedNumber(string const & number);
  };

  /// @return \n
  /// -1 - no match;
  ///  0 - full match;
  ///  1 - integer number match with odd (even).
  ///  2 - integer number match.
  int GetMatch(ParsedNumber const & number) const;
};

struct HouseProjection
{
  House const * m_house;
  m2::PointD m_proj;
  double m_distance;
  /// Distance in mercator, from street beginning to projection on street
  double m_streetDistance;
  /// false - to the left, true - to the right from projection segment
  bool m_projectionSign;

  inline static bool LessDistance(HouseProjection const & r1, HouseProjection const & r2)
  {
    return r1.m_distance < r2.m_distance;
  }
};

// many features combines to street
class Street
{
  string m_name;
  string m_processedName;

public:
  void SetName(string const & name);
  string const & GetName() const { return m_name; }

  vector<m2::PointD> m_points;
  vector<HouseProjection> m_houses;
  bool m_housesReaded;
  int m_number;

  Street() : m_housesReaded(false), m_number(-1) {}

  void SortHousesProjection();

  /// Get limit rect for street with ortho offset to the left and right.
  m2::RectD GetLimitRect(double offsetMeters) const;

  inline static bool IsSameStreets(Street const * s1, Street const * s2)
  {
    return s1->m_processedName == s2->m_processedName;
  }

  inline static string GetDbgName(vector<Street *> const & streets)
  {
    return streets.front()->m_processedName;
  }
};

class HouseDetector
{
  FeatureLoader m_loader;

  map<FeatureID, Street *> m_id2st;
  map<FeatureID, House *> m_id2house;

public:
  class LessWithEpsilon
  {
    double * m_eps;
  public:
    LessWithEpsilon(double * eps) : m_eps(eps) {}
    bool operator() (m2::PointD const & p1, m2::PointD const & p2) const
    {
      if (p1.x + *m_eps < p2.x)
        return true;
      else if (p2.x + *m_eps < p1.x)
        return false;
      else
        return (p1.y + *m_eps < p2.y);
    }
  };

private:
  double m_epsMercator;
  typedef multimap<m2::PointD, Street *, LessWithEpsilon>::iterator IterT;
  multimap<m2::PointD, Street *, LessWithEpsilon> m_end2st;
  vector<vector<Street *> > m_streets;

  int m_streetNum;

  void FillQueue(queue<Street *> & q, Street const * street, bool isBeg);
  void Bfs(Street * st);

  template <class ProjectionCalcT>
  void ReadHouse(FeatureType const & f, Street * st, ProjectionCalcT & calc);

  void SetMetres2Mercator(double factor);

public:
  typedef map<FeatureID, Street *>::iterator IterM;

  HouseDetector(Index const * pIndex);

  int LoadStreets(vector<FeatureID> const & ids);
  /// @return number of different joined streets.
  int MergeStreets();

  void ReadHouses(Street * st, double offsetMeters);
  void ReadAllHouses(double offsetMeters);

  void GetHouseForName(string const & houseNumber, vector<House const *> & res);

  void ClearCaches();
};

}
