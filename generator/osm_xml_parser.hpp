#pragma once

#include "base/exception.hpp"

#include "geometry/point2d.hpp"

#include "std/vector.hpp"
#include "std/map.hpp"
#include "std/set.hpp"
#include "std/string.hpp"

namespace osm
{
  typedef int64_t OsmId;
  typedef vector<OsmId> OsmIds;

  typedef pair<string, string> OsmTag;
  typedef vector<OsmTag> OsmTags;

  struct RelationMember
  {
    OsmId m_ref;
    string m_type;
    string m_role;
  };
  typedef vector<RelationMember> RelationMembers;

  class OsmIdAndTagHolder
  {
    OsmId m_id;
    OsmTags m_tags;

  public:
    OsmIdAndTagHolder(OsmId id, OsmTags const & tags);
    OsmId Id() const { return m_id; }
    bool TagValueByKey(string const & key, string & outValue) const;
    template <class TFunctor> void ForEachTag(TFunctor & functor) const
    {
      for (OsmTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
        functor(*it);
    }
  };

  class OsmNode : public OsmIdAndTagHolder
  {
  public:
    double m_lat;
    double m_lon;
    OsmNode(OsmId id, OsmTags const & tags, double lat, double lon);
  };

  class OsmWay : public OsmIdAndTagHolder
  {
    OsmIds m_points;

  public:
    OsmWay(OsmId id, OsmTags const & tags, OsmIds const & pointIds);

    size_t PointsCount() const;

    /// checks if first and last points are equal
    bool IsClosed() const;

    /// Merges ways if they have one common point
    /// @warning do not use it where merged way direction is important! (coastlines for example)
    bool MergeWith(OsmWay const & way);

    template <class TFunctor> void ForEachPoint(TFunctor & functor) const
    {
      for (typename OsmIds::const_iterator it = m_points.begin(); it != m_points.end(); ++it)
        functor(*it);
    }
  };

  typedef vector<OsmWay> OsmWays;

  class OsmRelation : public OsmIdAndTagHolder
  {
    RelationMembers m_members;

  public:
    OsmRelation(OsmId id, OsmTags const & tags, RelationMembers const & members);
    OsmIds MembersByTypeAndRole(string const & type, string const & role) const;
  };

  class OsmRawData
  {
    typedef map<OsmId, OsmNode> nodes_type;
    nodes_type m_nodes;
    typedef map<OsmId, OsmWay> ways_type;
    ways_type m_ways;
    typedef map<OsmId, OsmRelation> relations_type;
    relations_type m_relations;

  public:
    DECLARE_EXCEPTION(OsmInvalidIdException, RootException);

    void AddNode(OsmId id, OsmTags const & tags, double lat, double lon);
    void AddWay(OsmId id, OsmTags const & tags, OsmIds const & nodeIds);
    void AddRelation(OsmId id, OsmTags const & tags, RelationMembers const & members);

    OsmNode NodeById(OsmId id) const;         //!< @throws OsmInvalidIdException
    OsmWay WayById(OsmId id) const;           //!< @throws OsmInvalidIdException
    OsmRelation RelationById(OsmId id) const; //!< @throws OsmInvalidIdException

    OsmIds RelationsByKey(string const & key) const;
    OsmIds RelationsByTag(OsmTag const & tag) const;
  };

  class OsmXmlParser
  {
    vector<string> m_xmlTags;
    /// true if action=delete or visible=false for corresponding xml tag
    vector<bool> m_invalidTags;

    OsmRawData & m_osmRawData;

    OsmId m_id;
    double m_lon;
    double m_lat;
    /// <tag k="..." v="..."/>
    string m_k;
    string m_v;
    OsmTags m_tags;
    /// <nd ref="..." />
    OsmId m_ref;
    OsmIds m_nds;
    RelationMember m_member;
    RelationMembers m_members;

  public:
    OsmXmlParser(OsmRawData & outData);

    bool Push(string const & element);
    void Pop(string const & element);
    void AddAttr(string const & attr, string const & value);
    void CharData(string const &) {}
  };

}
