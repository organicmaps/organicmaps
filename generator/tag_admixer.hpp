#pragma once

#include "generator/osm_element.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>


class WaysParserHelper
{
public:
  explicit WaysParserHelper(std::map<uint64_t, std::string> & ways) : m_ways(ways) {}

  void ParseStream(std::istream & input)
  {
    std::string oneLine;
    while (std::getline(input, oneLine, '\n'))
    {
      // String format: <<id;tag>>.
      auto pos = oneLine.find(';');
      if (pos != std::string::npos)
      {
        uint64_t wayId;
        CHECK(strings::to_uint64(oneLine.substr(0, pos), wayId),());
        m_ways[wayId] = oneLine.substr(pos + 1, oneLine.length() - pos - 1);
      }
    }
  }

private:
  std::map<uint64_t, std::string> & m_ways;
};

class CapitalsParserHelper
{
public:
  explicit CapitalsParserHelper(std::set<uint64_t> & capitals) : m_capitals(capitals) {}

  void ParseStream(std::istream & input)
  {
    std::string oneLine;
    while (std::getline(input, oneLine, '\n'))
    {
      // String format: <<lat;lon;id;is_capital>>.
      // First ';'.
      auto pos = oneLine.find(";");
      if (pos != std::string::npos)
      {
        // Second ';'.
        pos = oneLine.find(";", pos + 1);
        if (pos != std::string::npos)
        {
          uint64_t nodeId;
          // Third ';'.
          auto endPos = oneLine.find(";", pos + 1);
          if (endPos != std::string::npos)
          {
            if (strings::to_uint64(oneLine.substr(pos + 1, endPos - pos - 1), nodeId))
              m_capitals.insert(nodeId);
          }
        }
      }
    }
  }

private:
  std::set<uint64_t> & m_capitals;
};

class TagAdmixer
{
public:
  TagAdmixer() = default;

  explicit TagAdmixer(std::string const & waysFile, std::string const & capitalsFile)
  {
    {
      std::ifstream reader(waysFile);
      WaysParserHelper parser(m_ways);
      parser.ParseStream(reader);
    }

    {
      std::ifstream reader(capitalsFile);
      CapitalsParserHelper parser(m_capitals);
      parser.ParseStream(reader);
    }
  }

  TagAdmixer(TagAdmixer const & other)
    : m_ways(other.m_ways), m_capitals(other.m_capitals) {}

  TagAdmixer & operator=(TagAdmixer const & other)
  {
    if (this != &other)
    {
      m_ways = other.m_ways;
      m_capitals = other.m_capitals;
    }

    return *this;
  }

  void Process(OsmElement & element) const
  {
    if (element.m_type == OsmElement::EntityType::Way)
    {
      auto const it = m_ways.find(element.m_id);
      if (it == m_ways.cend())
        return;

      // Exclude ferry routes.
      static OsmElement::Tag const kFerryTag = {"route", "ferry"};
      auto const & tags = element.Tags();
      if (std::find(tags.cbegin(), tags.cend(), kFerryTag) == tags.cend())
        element.AddTag("highway", it->second);
    }
    else if (element.m_type == OsmElement::EntityType::Node &&
             m_capitals.find(element.m_id) != m_capitals.cend())
    {
      // Our goal here - to make some capitals visible in World map.
      // The simplest way is to upgrade population to 45000,
      // according to our visibility rules in mapcss files.
      element.UpdateTag("population", [] (std::string & v)
      {
        uint64_t n;
        if (!strings::to_uint64(v, n) || n < 45000)
          v = "45000";
      });
    }
  }

private:
  std::map<uint64_t, std::string> m_ways;
  std::set<uint64_t> m_capitals;
};

class TagReplacer
{
public:
  TagReplacer() = default;

  explicit TagReplacer(std::string const & filePath)
  {
    std::ifstream stream(filePath);

    OsmElement::Tag tag;
    std::vector<std::string> values;
    std::string line;
    while (std::getline(stream, line))
    {
      if (line.empty())
        continue;

      strings::SimpleTokenizer iter(line, " \t=,:");
      if (!iter)
        continue;
      tag.m_key = *iter;
      ++iter;
      if (!iter)
        continue;
      tag.m_value = *iter;

      values.clear();
      while (++iter)
        values.push_back(*iter);

      if (values.size() >= 2 && values.size() % 2 == 0)
        m_entries[tag].swap(values);
    }
  }

  TagReplacer(TagReplacer const & other) : m_entries(other.m_entries) {}

  TagReplacer & operator=(TagReplacer const & other)
  {
    if (this != &other)
      m_entries = other.m_entries;

    return *this;
  }

  void Process(OsmElement & element) const
  {
    for (auto & tag : element.m_tags)
    {
      auto const it = m_entries.find(tag);
      if (it != m_entries.end())
      {
        auto const & v = it->second;
        tag.m_key = v[0];
        tag.m_value = v[1];
        for (size_t i = 2; i < v.size(); i += 2)
          element.AddTag(v[i], v[i + 1]);
      }
    }
  }

private:
  std::map<OsmElement::Tag, std::vector<std::string>> m_entries;
};

class OsmTagMixer
{
public:
  OsmTagMixer() = default;

  explicit OsmTagMixer(std::string const & filePath)
  {
    std::ifstream stream(filePath);
    std::vector<std::string> values;
    std::vector<OsmElement::Tag> tags;
    std::string line;
    while (std::getline(stream, line))
    {
      if (line.empty() || line.front() == '#')
        continue;

      strings::ParseCSVRow(line, ',', values);
      if (values.size() < 3)
        continue;

      OsmElement::EntityType entityType = OsmElement::StringToEntityType(values[0]);
      uint64_t id;
      if (entityType == OsmElement::EntityType::Unknown || !strings::to_uint64(values[1], id))
        continue;

      for (size_t i = 2; i < values.size(); ++i)
      {
        auto p = values[i].find('=');
        if (p != std::string::npos)
          tags.push_back(OsmElement::Tag(values[i].substr(0, p), values[i].substr(p + 1)));
      }

      if (!tags.empty())
        m_elements[{entityType, id}].swap(tags);
    }
  }

  OsmTagMixer(OsmTagMixer const & other) : m_elements(other.m_elements) {}

  OsmTagMixer & operator=(OsmTagMixer const & other)
  {
    if (this != &other)
      m_elements = other.m_elements;

    return *this;
  }

  void Process(OsmElement & element) const
  {
    auto const elements = m_elements.find({element.m_type, element.m_id});
    if (elements != m_elements.end())
    {
      for (OsmElement::Tag const & tag : elements->second)
        element.UpdateTag(tag.m_key, [&tag](std::string & v) { v = tag.m_value; });
    }
  }

private:
  std::map<std::pair<OsmElement::EntityType, uint64_t>, std::vector<OsmElement::Tag>> m_elements;
};
