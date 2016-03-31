#pragma once

#include "generator/osm_element.hpp"

#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/fstream.hpp"
#include "std/map.hpp"
#include "std/string.hpp"


class WaysParserHelper
{
public:
  WaysParserHelper(map<uint64_t, string> & ways) : m_ways(ways) {}

  void ParseStream(istream & input)
  {
    string oneLine;
    while (getline(input, oneLine, '\n'))
    {
      // String format: <<id;tag>>.
      auto pos = oneLine.find(';');
      if (pos != string::npos)
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
      // String format: <<lat;lon;id;is_capital>>.
      // First ';'.
      auto pos = oneLine.find(";");
      if (pos != string::npos)
      {
        // Second ';'.
        pos = oneLine.find(";", pos + 1);
        if (pos != string::npos)
        {
          uint64_t nodeId;
          // Third ';'.
          auto endPos = oneLine.find(";", pos + 1);
          if (endPos != string::npos)
          {
            if (strings::to_uint64(oneLine.substr(pos + 1, endPos - pos - 1), nodeId))
              m_capitals.insert(nodeId);
          }
        }
      }
    }
  }

private:
  set<uint64_t> & m_capitals;
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
      LOG(LWARNING, ("Can't read the world level capitals file! Generating world without towns admixing. Path:", capitalsFile));
      return;
    }
  }

  void operator()(OsmElement * e)
  {
    if (e->type == OsmElement::EntityType::Way && m_ways.find(e->id) != m_ways.end())
    {
      // Exclude ferry routes.
      if (find(e->Tags().begin(), e->Tags().end(), m_ferryTag) == e->Tags().end())
        e->AddTag("highway", m_ways[e->id]);
    }
    else if (e->type == OsmElement::EntityType::Node && m_capitals.find(e->id) != m_capitals.end())
    {
      // Our goal here - to make some capitals visible in World map.
      // The simplest way is to upgrade population to 45000,
      // according to our visibility rules in mapcss files.
      e->UpdateTag("population", [] (string & v)
      {
        uint64_t n;
        if (!strings::to_uint64(v, n) || n < 45000)
          v = "45000";
      });
    }
  }

private:
  map<uint64_t, string> m_ways;
  set<uint64_t> m_capitals;
  OsmElement::Tag const m_ferryTag;
};

class TagReplacer
{
  vector<vector<string>> m_entries;
public:
  TagReplacer(string const & filePath)
  {
    try
    {
      ifstream stream(filePath);
      while (stream.good())
      {
        string line;
        std::getline(stream, line);
        if (line.empty())
          continue;

        vector<string> v;
        strings::Tokenize(line, " \t=,:", MakeBackInsertFunctor(v));
        if (v.size() < 4 || v.size() % 2 == 1)
          continue;

        m_entries.push_back(move(v));
      }
    }
    catch (ifstream::failure const &)
    {
      LOG(LWARNING, ("Can't read replacing tags info file", filePath));
    }
  }

  void operator()(OsmElement * p)
  {
    for (auto & tag : p->m_tags)
    {
      for (auto const & entry : m_entries)
      {
        if (tag.key == entry[0] && tag.value == entry[1])
        {
          tag.key = entry[2];
          tag.value = entry[3];
          for (size_t i = 4; i < entry.size(); i += 2)
            p->AddTag(entry[i], entry[i+1]);
        }
      }
    }
  }
};
