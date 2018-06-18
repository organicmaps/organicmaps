#pragma once

#include "search/feature_loader.hpp"
#include "search/projection_on_street.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include "std/string.hpp"
#include "std/queue.hpp"

class DataSourceBase;

namespace search
{
struct ParsedNumber
{
  string m_fullN;
  int m_startN, m_endN;

public:
  /// @todo Pass correct "American" notation flag.
  ParsedNumber(string const & number, bool american = false);

  inline string const & GetNumber() const { return m_fullN; }
  inline bool IsOdd() const { return (m_startN % 2 == 1); }
  inline int GetIntNumber() const { return m_startN; }

  bool IsIntersect(ParsedNumber const & number, int offset = 0) const;
};

class House
{
  ParsedNumber m_number;
  m2::PointD m_point;

public:
  House(string const & number, m2::PointD const & point)
    : m_number(number), m_point(point)
  {
  }

  inline string const & GetNumber() const { return m_number.GetNumber(); }
  inline int GetIntNumber() const { return m_number.GetIntNumber(); }
  inline m2::PointD const & GetPosition() const { return m_point; }

  /// @return \n
  /// -1 - no match;
  ///  0 - full match;
  ///  1 - integer number match with odd (even).
  ///  2 - integer number match.
  int GetMatch(ParsedNumber const & number) const;
  bool GetNearbyMatch(ParsedNumber const & number) const;
};

// NOTE: DO NOT DELETE instances of this class by a pointer/reference
// to ProjectionOnStreet, because both classes have non-virtual destructors.
struct HouseProjection : public ProjectionOnStreet
{
  House const * m_house;

  /// Distance in mercator, from street beginning to projection on street
  double m_streetDistance;

  inline bool IsOdd() const { return (m_house->GetIntNumber() % 2 == 1); }

  struct LessDistance
  {
    bool operator() (HouseProjection const * p1, HouseProjection const * p2) const
    {
      return p1->m_distMeters < p2->m_distMeters;
    }
  };

  class EqualHouse
  {
    House const * m_house;
  public:
    EqualHouse(House const * h) : m_house(h) {}
    bool operator() (HouseProjection const * p) const { return m_house == p->m_house; }
  };
};

// many features combines to street
class Street
{
  string m_name;
  string m_processedName;

public:
  void SetName(string const & name);

  vector<m2::PointD> m_points;
  vector<HouseProjection> m_houses;
  double m_length;      /// Length in mercator
  int m_number;         /// Some ordered number after merging
  bool m_housesRead;

  Street() : m_length(0.0), m_number(-1), m_housesRead(false) {}

  void Reverse();
  void SortHousesProjection();

  /// Get limit rect for street with ortho offset to the left and right.
  m2::RectD GetLimitRect(double offsetMeters) const;

  double GetLength() const;

  double GetPrefixLength(size_t numSegs) const;

  inline static bool IsSameStreets(Street const * s1, Street const * s2)
  {
    return s1->m_processedName == s2->m_processedName;
  }

  inline string const & GetDbgName() const { return m_processedName; }
  inline string const & GetName() const { return m_name; }
};

class MergedStreet
{
  double m_length;
public:
  deque<Street *> m_cont;

  MergedStreet() : m_length(0.0) {}

  string const & GetDbgName() const;
  string const & GetName() const;
  bool IsHousesRead() const;
  void FinishReadingHouses();

  HouseProjection const * GetHousePivot(bool isOdd, bool & sign) const;

  struct GreaterLength
  {
    bool operator() (MergedStreet const & s1, MergedStreet const & s2) const
    {
      return (s1.m_length > s2.m_length);
    }
  };

  inline void Swap(MergedStreet & s)
  {
    m_cont.swap(s.m_cont);
    std::swap(m_length, s.m_length);
  }

public:
  struct Index
  {
    size_t s, h;
    Index() : s(0), h(0) {}
  };

  inline Index Begin() const
  {
    Index i;
    Next(i);
    return i;
  }
  inline void Inc(Index & i) const
  {
    ++i.h;
    Next(i);
  }
  inline bool IsEnd(Index const & i) const
  {
    return i.s == m_cont.size();
  }
  inline HouseProjection const & Get(Index const & i) const
  {
    ASSERT(!IsEnd(i), ());
    return m_cont[i.s]->m_houses[i.h];
  }

private:
  void Erase(Index & i);
  void Next(Index & i) const;
};

inline void swap(MergedStreet & s1, MergedStreet & s2)
{
  s1.Swap(s2);
}

struct HouseResult
{
  House const * m_house;
  MergedStreet const * m_street;

  HouseResult(House const * house, MergedStreet const * street) : m_house(house), m_street(street)
  {
  }

  inline bool operator<(HouseResult const & a) const { return m_house < a.m_house; }
  inline bool operator==(HouseResult const & a) const { return m_house == a.m_house; }

  m2::PointD const & GetOrg() const { return m_house->GetPosition(); }
};

inline string DebugPrint(HouseResult const & r)
{
  return r.m_house->GetNumber() + ", " + r.m_street->GetName();
}

class HouseDetector
{
  FeatureLoader m_loader;

  typedef map<FeatureID, Street *> StreetMapT;
  StreetMapT m_id2st;
  typedef map<FeatureID, House *> HouseMapT;
  HouseMapT m_id2house;

  vector<pair<m2::PointD, Street *> > m_end2st;
  vector<MergedStreet> m_streets;

  double m_metres2Mercator;
  int m_streetNum;
  double m_houseOffsetM;

  typedef pair<Street *, bool> StreetPtr;
  StreetPtr FindConnection(Street const * st, bool beg) const;
  void MergeStreets(Street * st);

  template <class ProjectionCalcT>
  void ReadHouse(FeatureType const & f, Street * st, ProjectionCalcT & calc);
  void ReadHouses(Street * st);

  void SetMetres2Mercator(double factor);

  double GetApprLengthMeters(int index) const;

public:
  HouseDetector(DataSourceBase const & index);
  ~HouseDetector();

  int LoadStreets(vector<FeatureID> const & ids);
  /// @return number of different joined streets.
  int MergeStreets();

  static int const DEFAULT_OFFSET_M = 200;
  void ReadAllHouses(double offsetMeters = DEFAULT_OFFSET_M);

  void GetHouseForName(string const & houseNumber, vector<HouseResult> & res);

  void ClearCaches();
  void ClearUnusedStreets(vector<FeatureID> const & ids);
};

}
