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
    while (std::getline(input, oneLine))
    {
      // String format: <<id;tag>>.
      auto pos = oneLine.find(';');
      if (pos != std::string::npos)
      {
        uint64_t wayId;
        CHECK(strings::to_uint64(oneLine.substr(0, pos), wayId), ());
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
    while (std::getline(input, oneLine))
    {
      // String format: <<lat;lon;id;is_capital>>.
      // First ';'.
      auto pos = oneLine.find(';');
      if (pos != std::string::npos)
      {
        // Second ';'.
        pos = oneLine.find(';', pos + 1);
        if (pos != std::string::npos)
        {
          uint64_t nodeId;
          // Third ';'.
          auto endPos = oneLine.find(';', pos + 1);
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

  void Process(OsmElement & element) const
  {
    if (element.m_type == OsmElement::EntityType::Way)
    {
      auto const it = m_ways.find(element.m_id);
      if (it == m_ways.cend())
        return;

      // Exclude ferry routes.
      /// @todo This routine is not used but anyway: what about railway-rail-motor_vehicle or route-shuttle_train?
      static OsmElement::Tag const kFerryTag = {"route", "ferry"};
      auto const & tags = element.Tags();
      if (!base::IsExist(tags, kFerryTag))
        element.AddTag("highway", it->second);
    }
    else if (element.m_type == OsmElement::EntityType::Node && m_capitals.find(element.m_id) != m_capitals.cend())
    {
      // Our goal here - to make some capitals visible in World map.
      // The simplest way is to upgrade population to 45000,
      // according to our visibility rules in mapcss files.
      element.UpdateTagFn("population", [](std::string & v)
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
  using Tag = OsmElement::Tag;
  struct ReplaceValue
  {
    std::vector<Tag> m_tags;
    bool m_update;
  };
  using Replacements = std::map<Tag, ReplaceValue>;

  TagReplacer() = default;

  explicit TagReplacer(std::string const & filePath)
  {
    std::ifstream stream(filePath);

    std::string line;
    size_t lineNumber = 0;
    while (std::getline(stream, line))
    {
      ++lineNumber;
      strings::Trim(line);
      if (line.empty() || line[0] == '#')
        continue;

      // find key
      auto keyPos = line.find('=');
      CHECK(keyPos != std::string::npos, ("Cannot find source tag key in", line));
      auto key = line.substr(0, keyPos);
      strings::Trim(key);

      // find value
      auto valuePos = line.find(':', ++keyPos);
      CHECK(valuePos != std::string::npos, ("Cannot find source tag value in", line));
      auto value = line.substr(keyPos, valuePos - keyPos);
      strings::Trim(value);

      // find trailing flag
      auto endPos = line.find('|', ++valuePos);
      bool isUpdate = false;
      if (endPos != std::string::npos)
      {
        // Can check flag value, but no need now ..
        isUpdate = true;
      }
      else
        endPos = line.size();

      // process replacement pairs
      CHECK(valuePos != std::string::npos, ("Cannot find source tag value in", line));
      auto res = m_replacements.emplace(Tag{key, value}, ReplaceValue{{}, isUpdate});
      CHECK(res.second, ());

      strings::Tokenize(line.substr(valuePos, endPos - valuePos), ",", [&](std::string_view token)
      {
        auto kv = strings::Tokenize<std::string>(token, "=");
        CHECK_EQUAL(kv.size(), 2, ("Cannot parse replacement tag:", token, "in line", lineNumber));
        strings::Trim(kv[0]);
        strings::Trim(kv[1]);
        res.first->second.m_tags.emplace_back(std::move(kv[0]), std::move(kv[1]));
      });
    }
  }

  void Process(OsmElement & element) const
  {
    std::vector<ReplaceValue const *> replace;
    base::EraseIf(element.m_tags, [&](auto const & tag)
    {
      auto const it = m_replacements.find(tag);
      if (it != m_replacements.end())
      {
        replace.push_back(&(it->second));
        return true;
      }
      return false;
    });

    for (auto const & r : replace)
    {
      for (auto const & tag : r->m_tags)
        if (r->m_update)
          element.UpdateTag(tag.m_key, tag.m_value);
        else
          element.AddTag(tag);
    }
  }

  std::vector<Tag> const * GetTagsForTests(Tag const & key) const
  {
    auto const it = m_replacements.find(key);
    return (it != m_replacements.end() ? &(it->second.m_tags) : nullptr);
  }

private:
  Replacements m_replacements;
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
      for (OsmElement::Tag const & tag : elements->second)
        element.UpdateTag(tag.m_key, tag.m_value);
  }

private:
  std::map<std::pair<OsmElement::EntityType, uint64_t>, std::vector<OsmElement::Tag>> m_elements;
};
