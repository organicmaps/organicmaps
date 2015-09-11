#include "indexer/drules_city_rank_table.hpp"
#include "indexer/scales.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/exception.hpp"
#include "std/function.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#include "base/assert.hpp"

namespace drule
{

namespace
{

using TZoomRange = pair<uint32_t, uint32_t>;
using TPopulationRange = pair<uint32_t, uint32_t>;
using TRankForPopulationRange = pair<double, TPopulationRange>;
using TCityRankVector = vector<TRankForPopulationRange>;
using TCityRankTable = unordered_map<uint32_t, TCityRankVector>;

class ConstCityRankTable : public ICityRankTable
{
public:
  explicit ConstCityRankTable(double rank)
    : m_rank(rank)
  {
  }

  bool GetCityRank(uint32_t /*zoomLevel*/, uint32_t /*population*/, double & rank) const override
  {
    rank = m_rank;
    return true;
  }

private:
  double const m_rank;
};

class CityRankTable : public ICityRankTable
{
public:
  explicit CityRankTable(TCityRankTable && table)
    : m_table(move(table))
  {
  }

  bool GetCityRank(uint32_t zoomLevel, uint32_t population, double & rank) const override
  {
    auto const itr = m_table.find(zoomLevel);
    if (itr == m_table.cend())
      return false;

    for (auto const & rp : itr->second)
    {
      if (population >= rp.second.first && population <= rp.second.second)
      {
        rank = rp.first;
        return true;
      }
    }

    return false;
  }

private:
  TCityRankTable const m_table;
};

void RemoveSpaces(string & s)
{    
  s.erase(remove_if(s.begin(), s.end(), &isspace), s.end());
}

bool ParseZoom(string const & s, size_t start, size_t end,
               uint32_t & zFrom, uint32_t & zTo)
{
  ASSERT_LESS(start, end, ());
  ASSERT_LESS(end, s.length(), ());

  // Parse Zoom
  // z[zoomFrom]-[zoomTo]
  // z[zoom]

  // length of Zoom must be more than 1 because it starts with z
  if ((end - start) <= 1)
    return false;

  if (s[start] != 'z')
    return false;

  string zFromStr, zToStr;

  size_t const d = s.find('-', start + 1);
  if (d == string::npos || d >= end)
  {
    zFromStr.assign(s.begin() + start + 1, s.begin() + end);
    zToStr = zFromStr;
  }
  else
  {
    zFromStr.assign(s.begin() + start + 1, s.begin() + d);
    zToStr.assign(s.begin() + d + 1, s.begin() + end);
  }

  try
  {
    zFrom = zFromStr.empty() ? 0 : stoi(zFromStr);
    zTo = zToStr.empty() ? scales::UPPER_STYLE_SCALE : stoi(zToStr);
  }
  catch (exception &)
  {
    // stoi has not parsed a number
    return false;
  }

  if (zFrom > zTo)
    return false;

  return true;
}

bool ParsePopulationRank(string const & s, size_t start, size_t end,
                         uint32_t & popFrom, uint32_t & popTo, double & rank)
{
  ASSERT_LESS(start, end, ());
  ASSERT_LESS(end, s.length(), ());

  // Parse PopulationRank
  // [populationFrom]-[populationTo]:rank

  size_t const d1 = s.find('-', start);
  if (d1 == string::npos || d1 >= end)
    return false;

  size_t const d2 = s.find(':', d1 + 1);
  if (d2 == string::npos || d2 >= end)
    return false;

  string popFromStr(s.begin() + start, s.begin() + d1);
  string popToStr(s.begin() + d1 + 1, s.begin() + d2);
  string rankStr(s.begin() + d2 + 1, s.begin() + end);

  try
  {
    popFrom = popFromStr.empty() ? 0 : stoi(popFromStr);
    popTo = popToStr.empty() ? numeric_limits<uint32_t>::max() : stoi(popToStr);
    rank = stod(rankStr);
  }
  catch (exception &)
  {
    // stoi, stod has not parsed a number
    return false;
  }

  if (popFrom > popTo)
    return false;

  return true;
}

bool ParsePopulationRanks(string const & s, size_t start, size_t end,
                          function<void(uint32_t, uint32_t, double)> const & onPopRank)
{
  ASSERT_LESS(start, end, ());
  ASSERT_LESS(end, s.length(), ());

  // Parse 0...n of PopulationRank, delimited by ;

  while (start < end)
  {
    size_t const i = s.find(';', start);
    if (i == string::npos || i >= end)
      return false;

    if (i > start)
    {
      uint32_t popFrom, popTo;
      double rank;
      if (!ParsePopulationRank(s, start, i, popFrom, popTo, rank))
        return false;

      onPopRank(popFrom, popTo, rank);
    }

    start = i + 1;
  }

  return true;
}

bool ParseCityRankTable(string const & s,
                        function<void(uint32_t, uint32_t)> const & onZoom,
                        function<void(uint32_t, uint32_t, double)> const & onPopRank)
{
  // CityRankTable string format is
  // z[zoomFrom]-[zoomTo] { [populationFrom]-[populationTo]:rank;[populationFrom]-[populationTo]:rank; }

  size_t const end = s.length();
  size_t start = 0;

  while (start < end)
  {
    size_t const i = s.find('{', start);
    if (i == string::npos)
      return false;

    size_t const j = s.find('}', i + 1);
    if (j == string::npos)
      return false;

    uint32_t zFrom, zTo;
    if (!ParseZoom(s, start, i, zFrom, zTo))
      return false;

    if (j > (i + 1))
    {
      onZoom(zFrom, zTo);

      if (!ParsePopulationRanks(s, i + 1, j, onPopRank))
        return false;
    }

    start = j + 1;
  }

  return true;
}

class CityRankTableBuilder
{
public:
  unique_ptr<ICityRankTable> Parse(string & str)
  {
    RemoveSpaces(str);

    auto onZoom = bind(&CityRankTableBuilder::OnZoom, this, _1, _2);
    auto onPopRank = bind(&CityRankTableBuilder::OnPopulationRank, this, _1, _2, _3);
    if (!ParseCityRankTable(str, onZoom, onPopRank))
      return unique_ptr<ICityRankTable>();

    Flush();

    return make_unique<CityRankTable>(move(m_cityRanksTable));
  }

private:
  void OnPopulationRank(uint32_t popFrom, uint32_t popTo, double rank)
  {
    m_cityRanksForZoomRange.emplace_back(make_pair(rank, make_pair(popFrom, popTo)));
  }
  void OnZoom(uint32_t zoomFrom, uint32_t zoomTo)
  {
    Flush();

    m_zoomRange = make_pair(zoomFrom, zoomTo);
  }
  void Flush()
  {
    if (!m_cityRanksForZoomRange.empty())
    {
      for (uint32_t z = m_zoomRange.first; z <= m_zoomRange.second; ++z)
      {
        TCityRankVector & dst = m_cityRanksTable[z];
        dst.insert(dst.end(), m_cityRanksForZoomRange.begin(), m_cityRanksForZoomRange.end());
      }
      m_cityRanksForZoomRange.clear();
    }
  }

  TZoomRange m_zoomRange;
  TCityRankVector m_cityRanksForZoomRange;
  TCityRankTable m_cityRanksTable;
};

}  // namespace

unique_ptr<ICityRankTable> GetCityRankTableFromString(string & str)
{
  return CityRankTableBuilder().Parse(str);
}

unique_ptr<ICityRankTable> GetConstRankCityRankTable(double rank)
{
  return unique_ptr<ICityRankTable>(new ConstCityRankTable(rank));
}

}  // namespace drule
