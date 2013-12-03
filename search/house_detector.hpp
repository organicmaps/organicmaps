#pragma once

#include "../geometry/point2d.hpp"

#include "../std/string.hpp"

namespace search
{

const double s_epsilon = 10.0 * 360.0 / 40E6;

class FeatureLoader
{
public:
};

class House
{
  string m_number;
  m2::PointD m_point;
public:
};

// many features combines to street
class Street
{
  string m_name;
  vector<m2::PointD> m_points;

  vector<House> m_houses;
  bool m_housesReaded;
};

class HouseDetector
{
  FeatureLoader m_loader;

  // id -> street
  //map<FeatureID, Street *> m_id2st;

public:
  struct LessWithEpsilon
  {
    //static double s_epsilon;

    bool operator() (m2::PointD const & p1, m2::PointD const & p2) const
    {
      if (p1.x + s_epsilon < p2.x)
        return true;
      else if (p2.x + s_epsilon < p1.x)
        return false;
      else
      {
        return (p1.y + s_epsilon < p2.y);
      }
    }
  };

private:
  // start, end point -> street
  multimap<m2::PointD, Street *, LessWithEpsilon> m_end2st;

  Street * GetNext(Street const *);
  Street * GetPrev(Street const *);

public:
  HouseDetector();
};

}
