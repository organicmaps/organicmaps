#pragma once

#include "search/feature_loader.hpp"
#include "search/projection_on_street.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/point2d.hpp"

#include "base/macros.hpp"

#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

class DataSource;

namespace search
{
struct ParsedNumber
{
public:
  /// @todo Pass correct "American" notation flag.
  ParsedNumber(std::string const & number, bool american = false);

  std::string const & GetNumber() const { return m_fullN; }
  bool IsOdd() const { return (m_startN % 2 == 1); }
  int GetIntNumber() const { return m_startN; }

  bool IsIntersect(ParsedNumber const & number, int offset = 0) const;

private:
  std::string m_fullN;
  int m_startN;
  int m_endN;
};

class House
{
public:
  House(std::string const & number, m2::PointD const & point) : m_number(number), m_point(point) {}

  std::string const & GetNumber() const { return m_number.GetNumber(); }
  int GetIntNumber() const { return m_number.GetIntNumber(); }
  m2::PointD const & GetPosition() const { return m_point; }

  /// @return \n
  /// -1 - no match;
  ///  0 - full match;
  ///  1 - integer number match with odd (even).
  ///  2 - integer number match.
  int GetMatch(ParsedNumber const & number) const;
  bool GetNearbyMatch(ParsedNumber const & number) const;

private:
  ParsedNumber m_number;
  m2::PointD m_point;
};

// NOTE: DO NOT DELETE instances of this class by a pointer/reference
// to ProjectionOnStreet, because both classes have non-virtual destructors.
struct HouseProjection : public ProjectionOnStreet
{
  struct LessDistance
  {
    bool operator()(HouseProjection const * p1, HouseProjection const * p2) const
    {
      return p1->m_distMeters < p2->m_distMeters;
    }
  };

  class EqualHouse
  {
  public:
    explicit EqualHouse(House const * h) : m_house(h) {}
    bool operator()(HouseProjection const * p) const { return m_house == p->m_house; }

  private:
    House const * m_house;
  };

  bool IsOdd() const { return (m_house->GetIntNumber() % 2 == 1); }

  House const * m_house;

  /// Distance in mercator, from street beginning to projection on street
  double m_streetDistance;
};

// many features combines to street
class Street
{
public:
  Street() : m_length(0.0), m_number(-1), m_housesRead(false) {}

  void Reverse();
  void SortHousesProjection();

  /// Get limit rect for street with ortho offset to the left and right.
  m2::RectD GetLimitRect(double offsetMeters) const;

  double GetLength() const;

  double GetPrefixLength(size_t numSegs) const;

  static bool IsSameStreets(Street const * s1, Street const * s2) { return s1->m_processedName == s2->m_processedName; }

  void SetName(std::string_view name);
  std::string const & GetDbgName() const { return m_processedName; }
  std::string const & GetName() const { return m_name; }

  std::vector<m2::PointD> m_points;
  std::vector<HouseProjection> m_houses;
  double m_length;  /// Length in mercator
  int m_number;     /// Some ordered number after merging
  bool m_housesRead;

private:
  std::string m_name;
  std::string m_processedName;
};

class MergedStreet
{
public:
  struct Index
  {
    size_t s, h;
    Index() : s(0), h(0) {}
  };

  struct GreaterLength
  {
    bool operator()(MergedStreet const & s1, MergedStreet const & s2) const { return (s1.m_length > s2.m_length); }
  };

  MergedStreet() : m_length(0.0) {}

  std::string const & GetDbgName() const;
  std::string const & GetName() const;
  bool IsHousesRead() const;
  void FinishReadingHouses();

  HouseProjection const * GetHousePivot(bool isOdd, bool & sign) const;

  void Swap(MergedStreet & s)
  {
    m_cont.swap(s.m_cont);
    std::swap(m_length, s.m_length);
  }

  Index Begin() const
  {
    Index i;
    Next(i);
    return i;
  }

  void Inc(Index & i) const
  {
    ++i.h;
    Next(i);
  }

  bool IsEnd(Index const & i) const { return i.s == m_cont.size(); }

  HouseProjection const & Get(Index const & i) const
  {
    ASSERT(!IsEnd(i), ());
    return m_cont[i.s]->m_houses[i.h];
  }

  std::deque<Street *> m_cont;

private:
  void Erase(Index & i);
  void Next(Index & i) const;

  double m_length;
};

struct HouseResult
{
  HouseResult(House const * house, MergedStreet const * street) : m_house(house), m_street(street) {}

  bool operator<(HouseResult const & a) const { return m_house < a.m_house; }
  bool operator==(HouseResult const & a) const { return m_house == a.m_house; }

  m2::PointD const & GetOrg() const { return m_house->GetPosition(); }

  House const * m_house;
  MergedStreet const * m_street;
};

class HouseDetector
{
public:
  using StreetMap = std::map<FeatureID, Street *>;
  using HouseMap = std::map<FeatureID, House *>;
  using StreetPtr = std::pair<Street *, bool>;

  static int const DEFAULT_OFFSET_M = 200;

  explicit HouseDetector(DataSource const & dataSource);
  ~HouseDetector();

  int LoadStreets(std::vector<FeatureID> const & ids);
  /// @return number of different joined streets.
  int MergeStreets();

  void ReadAllHouses(double offsetMeters = DEFAULT_OFFSET_M);

  void GetHouseForName(std::string const & houseNumber, std::vector<HouseResult> & res);

  void ClearCaches();
  void ClearUnusedStreets(std::vector<FeatureID> const & ids);

private:
  StreetPtr FindConnection(Street const * st, bool beg) const;

  void MergeStreets(Street * st);

  template <typename ProjectionCalculator>
  void ReadHouse(FeatureType & f, Street * st, ProjectionCalculator & calc);

  void ReadHouses(Street * st);

  void SetMetersToMercator(double factor);

  double GetApprLengthMeters(int index) const;

  FeatureLoader m_loader;

  StreetMap m_id2st;
  HouseMap m_id2house;

  std::vector<std::pair<m2::PointD, Street *>> m_end2st;
  std::vector<MergedStreet> m_streets;

  double m_metersToMercator = 0.0;
  int m_streetNum = 0;
  double m_houseOffsetM = 0.0;
};

std::string DebugPrint(HouseProjection const & p);
std::string DebugPrint(HouseResult const & r);
}  // namespace search
