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

struct House
{
  string m_number;
  m2::PointD m_point;
};

struct HouseProjection
{
  House const * m_house;
  m2::PointD m_proj;
  double m_distance;

  static bool LessDistance(HouseProjection const & r1, HouseProjection const & r2)
  {
    return r1.m_distance < r2.m_distance;
  }
};

// many features combines to street
struct Street
{
  string m_name;
  vector<m2::PointD> m_points;

  vector<HouseProjection> m_houses;
  bool m_housesReaded;
  int m_number;

  Street() : m_housesReaded(false), m_number(-1) {}

  /// Get limit rect for street with ortho offset to the left and right.
  m2::RectD GetLimitRect(double offsetMeters) const;
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

  int m_streetNum;

  void FillQueue(queue<Street *> & q, Street const * street, bool isBeg);
  void Bfs(Street * st);

  template <class ProjectionCalcT>
  void ReadHouse(FeatureType const & f, Street * st, ProjectionCalcT & calc);

  void SetMetres2Mercator(double factor);

public:
  HouseDetector(Index const * pIndex);

  void LoadStreets(vector<FeatureID> & ids);
  /// @return number of different joined streets.
  int MergeStreets();

  void ReadHouses(Street * st, double offsetMeters);
  void ReadAllHouses(double offsetMeters);

  void MatchAllHouses(string const & houseNumber, vector<HouseProjection> & res);
};

}
