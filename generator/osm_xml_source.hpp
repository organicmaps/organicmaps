#pragma once

#include "generator/osm_element.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <functional>
#include <string>
#include <utility>

class XMLSource
{
public:
  using Emitter = std::function<void(OsmElement &&)>;

  XMLSource(Emitter && fn) : m_emitter(std::move(fn)) {}

  void CharData(std::string const &) {}

  using StringPtrT = char const *;

  void AddAttr(StringPtrT k, StringPtrT value)
  {
    if (!m_current)
      return;

    std::string_view key(k);

    if (key == "id")
      CHECK(strings::to_uint64(value, m_current->m_id), ("Unknown element with invalid id:", value));
    else if (key == "lon")
      CHECK(strings::to_double(value, m_current->m_lon), ("Bad node lon:", value));
    else if (key == "lat")
      CHECK(strings::to_double(value, m_current->m_lat), ("Bad node lat:", value));
    else if (key == "ref")
      CHECK(strings::to_uint64(value, m_current->m_ref), ("Bad node ref in way:", value));
    else if (key == "k")
      m_current->m_k = value;
    else if (key == "v")
      m_current->m_v = value;
    else if (key == "type")
      m_current->m_memberType = OsmElement::StringToEntityType(value);
    else if (key == "role")
      m_current->m_role = value;
  }

  bool Push(StringPtrT tagName)
  {
    ASSERT(tagName[0] && tagName[1], ());

    // Use first two chars as tagKey from tagName.
    /// @todo Well, if something goes wrong here, and below, the new OSM tag was added into .osm file dump.
    auto const tagKey = OsmElement::EntityType(*reinterpret_cast<uint16_t const *>(tagName));

    switch (++m_depth)
    {
    case 1: m_current = nullptr; break;
    case 2:
      m_current = &m_parent;
      m_current->m_type = tagKey;
      break;
    default:
      m_current = &m_child;
      m_current->m_type = tagKey;
      break;
    }
    return true;
  }

  void Pop(StringPtrT)
  {
    switch (--m_depth)
    {
    case 0: break;

    case 1:
      // Skip useless tags. See XMLSource::Push function above.
      if (m_current->m_type != OsmElement::EntityType::Bounds)
      {
        ASSERT_EQUAL(m_current, &m_parent, ());
        m_emitter(std::move(m_parent));
      }
      m_parent.Clear();
      break;

    default:
      switch (m_child.m_type)
      {
      case OsmElement::EntityType::Member:
        m_parent.AddMember(m_child.m_ref, m_child.m_memberType, m_child.m_role);
        break;
      case OsmElement::EntityType::Tag: m_parent.AddTag(m_child.m_k, m_child.m_v); break;
      case OsmElement::EntityType::Nd: m_parent.AddNd(m_child.m_ref); break;
      default: break;
      }
      m_current = &m_parent;
      m_child.Clear();
    }
  }

private:
  OsmElement m_parent;
  OsmElement m_child;

  size_t m_depth = 0;
  OsmElement * m_current = nullptr;

  Emitter m_emitter;
};
