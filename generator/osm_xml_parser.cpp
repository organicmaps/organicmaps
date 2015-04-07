#include "generator/osm_xml_parser.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

namespace osm
{
  OsmIdAndTagHolder::OsmIdAndTagHolder(OsmId id, OsmTags const & tags)
    : m_id(id), m_tags(tags)
  {
  }

  bool OsmIdAndTagHolder::TagValueByKey(string const & key, string & outValue) const
  {
    for (OsmTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    {
      if (it->first == key)
      {
        outValue = it->second;
        return true;
      }
    }
    return false;
  }

  /////////////////////////////////////////////////////////
  OsmNode::OsmNode(OsmId id, OsmTags const & tags, double lat, double lon)
    : OsmIdAndTagHolder(id, tags), m_lat(lat), m_lon(lon)
  {
  }

  /////////////////////////////////////////////////////////

  OsmWay::OsmWay(OsmId id, OsmTags const & tags, OsmIds const & pointIds)
    : OsmIdAndTagHolder(id, tags), m_points(pointIds)
  {
    CHECK_GREATER_OR_EQUAL(m_points.size(), 2, ("Can't construct a way with less than 2 points", id));
  }

  size_t OsmWay::PointsCount() const
  {
    return m_points.size();
  }

  bool OsmWay::IsClosed() const
  {
    return m_points.front() == m_points.back();
  }

  bool OsmWay::MergeWith(OsmWay const & way)
  {
    size_t const oldSize = m_points.size();

    // first, try to connect our end with their start
    if (m_points.back() == way.m_points.front())
      m_points.insert(m_points.end(), ++way.m_points.begin(), way.m_points.end());
    // our end and their end
    else if (m_points.back() == way.m_points.back())
      m_points.insert(m_points.end(), ++way.m_points.rbegin(), way.m_points.rend());
    // our start and their end
    else if (m_points.front() == way.m_points.back())
      m_points.insert(m_points.begin(), way.m_points.begin(), --way.m_points.end());
    // our start and their start
    else if (m_points.front() == way.m_points.front())
      m_points.insert(m_points.begin(), way.m_points.rbegin(), --way.m_points.rend());

    return m_points.size() > oldSize;
  }

  ///////////////////////////////////////////////////////////////

  OsmRelation::OsmRelation(OsmId id, OsmTags const & tags, RelationMembers const & members)
    : OsmIdAndTagHolder(id, tags), m_members(members)
  {
    CHECK(!m_members.empty(), ("Can't construct a relation without members", id));
  }

  OsmIds OsmRelation::MembersByTypeAndRole(string const & type, string const & role) const
  {
    OsmIds result;
    for (RelationMembers::const_iterator it = m_members.begin(); it != m_members.end(); ++it)
      if (it->m_type == type && it->m_role == role)
        result.push_back(it->m_ref);
    return result;
  }

  ///////////////////////////////////////////////////////////////

  void OsmRawData::AddNode(OsmId id, OsmTags const & tags, double lat, double lon)
  {
    m_nodes.insert(nodes_type::value_type(id, OsmNode(id, tags, lat, lon)));
  }

  void OsmRawData::AddWay(OsmId id, OsmTags const & tags, OsmIds const & nodeIds)
  {
    m_ways.insert(ways_type::value_type(id, OsmWay(id, tags, nodeIds)));
  }

  void OsmRawData::AddRelation(OsmId id, OsmTags const & tags, RelationMembers const & members)
  {
    m_relations.insert(relations_type::value_type(id, OsmRelation(id, tags, members)));
  }

  OsmNode OsmRawData::NodeById(OsmId id) const
  {
    nodes_type::const_iterator found = m_nodes.find(id);
    if (found == m_nodes.end())
      MYTHROW( OsmInvalidIdException, (id, "node not found") );
    return found->second;
  }

  OsmWay OsmRawData::WayById(OsmId id) const
  {
    ways_type::const_iterator found = m_ways.find(id);
    if (found == m_ways.end())
      MYTHROW( OsmInvalidIdException, (id, "way not found") );
    return found->second;
  }

  OsmRelation OsmRawData::RelationById(OsmId id) const
  {
    relations_type::const_iterator found = m_relations.find(id);
    if (found == m_relations.end())
      MYTHROW( OsmInvalidIdException, (id, "relation not found") );
    return found->second;
  }

  OsmIds OsmRawData::RelationsByKey(string const & key) const
  {
    OsmIds result;
    string value;
    for (relations_type::const_iterator it = m_relations.begin(); it != m_relations.end(); ++it)
    {
      if (it->second.TagValueByKey(key, value))
        result.push_back(it->first);
    }
    return result;
  }

  OsmIds OsmRawData::RelationsByTag(OsmTag const & tag) const
  {
    OsmIds result;
    string value;
    for (relations_type::const_iterator it = m_relations.begin(); it != m_relations.end(); ++it)
    {
      if (it->second.TagValueByKey(tag.first, value) && value == tag.second)
        result.push_back(it->first);
    }
    return result;
  }

  /////////////////////////////////////////////////////////

  OsmXmlParser::OsmXmlParser(OsmRawData & outData)
    : m_osmRawData(outData)
  {
  }

  bool OsmXmlParser::Push(string const & element)
  {
    m_xmlTags.push_back(element);
    m_invalidTags.push_back(false);

    return true;
  }

  void OsmXmlParser::Pop(string const & element)
  {
    bool invalid = m_invalidTags.back();
    if (element == "node")
    {
      if (!invalid)
        m_osmRawData.AddNode(m_id, m_tags, m_lat, m_lon);
      m_tags.clear();
    }
    else if (element == "nd")
    {
      if (!invalid)
        m_nds.push_back(m_ref);
    }
    else if (element == "way")
    {
      if (!invalid)
        m_osmRawData.AddWay(m_id, m_tags, m_nds);
      m_nds.clear();
      m_tags.clear();
    }
    else if (element == "tag")
    {
      if (!invalid)
        m_tags.push_back(OsmTag(m_k, m_v));
    }
    else if (element == "member")
    {
      if (!invalid)
        m_members.push_back(m_member);
    }
    else if (element == "relation")
    {
      if (!invalid)
        m_osmRawData.AddRelation(m_id, m_tags, m_members);
      m_members.clear();
      m_tags.clear();
    }

    m_invalidTags.pop_back();
    m_xmlTags.pop_back();
  }

  void OsmXmlParser::AddAttr(string const & attr, string const & value)
  {
    string const & elem = m_xmlTags.back();
    if (attr == "id" && (elem == "node" || elem == "way" || elem == "relation"))
    {
      CHECK(strings::to_int64(value, m_id), ());
      CHECK_NOT_EQUAL(m_id, 0, ("id == 0 is invalid"));
    }
    else if (attr == "lat" && elem == "node")
    {
      CHECK(strings::to_double(value, m_lat), ());
    }
    else if (attr == "lon" && elem == "node")
    {
      CHECK(strings::to_double(value, m_lon), ());
    }
    else if (attr == "ref")
    {
      int64_t numVal;
      CHECK(strings::to_int64(value, numVal), ());
      if (elem == "nd")
        m_ref = numVal;
      else if (elem == "member")
        m_member.m_ref = numVal;
    }
    else if (attr == "k" && elem == "tag")
    {
      m_k = value;
    }
    else if (attr == "v" && elem == "tag")
    {
      m_v = value;
    }
    else if (attr == "type" && elem == "member")
    {
      m_member.m_type = value;
    }
    else if (attr == "role" && elem == "member")
    {
      m_member.m_role = value;
    }
    else if ((attr == "action" && value == "delete")
             || (attr == "visible" && value == "false"))
    {
      m_invalidTags.back() = true;
    }
  }

}
