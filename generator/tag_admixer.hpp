#pragma once

#include "generator/osm_element.hpp"

#include "coding/file_reader.hpp"

#include "std/map.hpp"
#include "std/string.hpp"

class WaysParserHelper
{
public:
  WaysParserHelper(map<uint64_t, string> & ways) : m_ways(ways) {}
  void ParseString(string const & input)
  {
    stringstream stream(input);

    string oneLine;
    while (getline(stream, oneLine, '\n'))
    {
      auto pos = oneLine.find(';');
      if (pos < oneLine.length())
      {
        uint64_t wayId = stoll(oneLine.substr(0, pos));
        m_ways[wayId] = oneLine.substr(pos + 1, oneLine.length() - pos - 1);
      }
    }
  }

private:
  map<uint64_t, string> & m_ways;
};

class TagAdmixer
{
public:
  TagAdmixer(string const & fileName) : m_ferryTag("route", "ferry")
  {
    string data;
    FileReader reader(fileName);
    reader.ReadAsString(data);
    WaysParserHelper parser(m_ways);
    parser.ParseString(data);
  }

  OsmElement * operator()(OsmElement * e)
  {
    if (e == nullptr)
      return e;
    if (e->type != OsmElement::EntityType::Way || m_ways.find(e->id) == m_ways.end())
      return e;

    // Exclude ferry routes.
    if (find(e->Tags().begin(), e->Tags().end(), m_ferryTag) != e->Tags().end())
      return e;

    e->AddTag("highway", m_ways[e->id]);
    return e;
  }

private:
  map<uint64_t, string> m_ways;
  OsmElement::Tag const m_ferryTag;
};
