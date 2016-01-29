#pragma once

#include "generator/osm_element.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/fstream.hpp"
#include "std/map.hpp"
#include "std/string.hpp"

namespace
{
  constexpr char const kPopulationTag[] = "population";
  constexpr char const kMinimalWorldLevelPopulation[] = "45000";
}  // namespace

class WaysParserHelper
{
public:
  WaysParserHelper(map<uint64_t, string> & ways) : m_ways(ways) {}

  void ParseStream(istream & input)
  {
    string oneLine;
    while (getline(input, oneLine, '\n'))
    {
      auto pos = oneLine.find(';');
      if (pos < oneLine.length())
      {
        uint64_t wayId;
        CHECK(strings::to_uint64(oneLine.substr(0, pos), wayId),());
        m_ways[wayId] = oneLine.substr(pos + 1, oneLine.length() - pos - 1);
      }
    }
  }

private:
  map<uint64_t, string> & m_ways;
};

class CapitalsParserHelper
{
public:
  CapitalsParserHelper(set<uint64_t> & capitals) : m_capitals(capitals) {}

  void ParseStream(istream & input)
  {
    string oneLine;
    while (getline(input, oneLine, '\n'))
    {
      auto pos = oneLine.find(";");
      if (pos < oneLine.length())
      {
        uint64_t nodeId;
        if (strings::to_uint64(oneLine.substr(0, pos), nodeId))
          m_capitals.insert(nodeId);
      }
    }
  }

private:
  set<uint64_t> m_capitals;
};

class TagAdmixer
{
public:
  TagAdmixer(string const & waysFile, string const & capitalsFile) : m_ferryTag("route", "ferry")
  {
    try
    {
      ifstream reader(waysFile);
      WaysParserHelper parser(m_ways);
      parser.ParseStream(reader);
    }
    catch (ifstream::failure const &)
    {
      LOG(LWARNING, ("Can't read the world level ways file! Generating world without roads. Path:", waysFile));
      return;
    }

    try
    {
      ifstream reader(capitalsFile);
      CapitalsParserHelper parser(m_capitals);
      parser.ParseStream(reader);
    }
    catch (ifstream::failure const &)
    {
      LOG(LWARNING, ("Can't read the world level capitals file! Generating world without roads. Path:", capitalsFile));
      return;
    }
  }

  OsmElement * operator()(OsmElement * e)
  {
    if (e == nullptr)
      return e;
    if (e->type == OsmElement::EntityType::Way && m_ways.find(e->id) != m_ways.end())
    {
      // Exclude ferry routes.
      if (find(e->Tags().begin(), e->Tags().end(), m_ferryTag) != e->Tags().end())
        return e;

      e->AddTag("highway", m_ways[e->id]);
      return e;
    }
    else if (e->type == OsmElement::EntityType::Node && m_capitals.find(e->id) != m_capitals.end())
    {
      if (!e->UpdateTag(kPopulationTag, kMinimalWorldLevelPopulation))
        e->AddTag(kPopulationTag, kMinimalWorldLevelPopulation);
    }
    return e;
  }

private:
  map<uint64_t, string> m_ways;
  set<uint64_t> m_capitals;
  OsmElement::Tag const m_ferryTag;
};
