#pragma once

#include "../indexer/feature_decl.hpp"
#include "../indexer/index.hpp"

#include "../geometry/point2d.hpp"

#include "../std/string.hpp"
#include "../std/queue.hpp"


namespace search
{

void GetStreetNameAsKey(string const & name, string & res);


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
  friend struct CompareHouseNumber;

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

  inline bool IsOdd() const { return (m_house->GetIntNumber() % 2 == 1); }

  struct LessDistance
  {
    bool operator() (HouseProjection const * p1, HouseProjection const * p2) const
    {
      return p1->m_distance < p2->m_distance;
    }
  };
};

struct CompareHouseNumber
{
  bool Less(HouseProjection const * h1, HouseProjection const * h2) const
  {
    return (h1->m_house->m_startN <= h2->m_house->m_startN);
  }
  bool Greater(HouseProjection const * h1, HouseProjection const * h2) const
  {
    return (h1->m_house->m_startN >= h2->m_house->m_startN);
  }
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
  bool m_housesReaded;

  Street() : m_length(0.0), m_number(-1), m_housesReaded(false) {}

  void Reverse();
  void SortHousesProjection();

  /// Get limit rect for street with ortho offset to the left and right.
  m2::RectD GetLimitRect(double offsetMeters) const;

  inline static bool IsSameStreets(Street const * s1, Street const * s2)
  {
    return s1->m_processedName == s2->m_processedName;
  }

  inline string const & GetDbgName() const { return m_processedName; }
};

class MergedStreet
{
  double m_length;
public:
  deque<Street *> m_cont;

  MergedStreet() : m_length(0.0) {}

  string const & GetDbgName() const;
  bool IsHousesReaded() const;
  void FinishReadingHouses();

  HouseProjection const * GetHousePivot(bool & isOdd, bool & sign) const;

  /// @name Temporary
  //@{
  inline size_t size() const { return m_cont.size(); }
  inline Street const * operator[] (size_t i) const { return m_cont[i]; }
  //@}

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

  typedef pair<Street *, bool> StreetPtr;
  StreetPtr FindConnection(Street const * st, bool beg) const;
  void MergeStreets(Street * st);

  template <class ProjectionCalcT>
  void ReadHouse(FeatureType const & f, Street * st, ProjectionCalcT & calc);
  void ReadHouses(Street * st, double offsetMeters);

  void SetMetres2Mercator(double factor);

  double GetApprLengthMeters(int index) const;

public:
  HouseDetector(Index const * pIndex);

  int LoadStreets(vector<FeatureID> const & ids);
  /// @return number of different joined streets.
  int MergeStreets();

  void ReadAllHouses(double offsetMeters);

  void GetHouseForName(string const & houseNumber, vector<House const *> & res);

  void ClearCaches();
};

}
