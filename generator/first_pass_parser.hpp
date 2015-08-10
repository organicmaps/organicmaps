#pragma once

#include "generator/xml_element.hpp"
#include "generator/osm_decl.hpp"

#include "indexer/mercator.hpp"

#include "base/string_utils.hpp"


template <class THolder>
class FirstPassParser : public BaseOSMParser
{
  THolder & m_holder;

public:
  FirstPassParser(THolder & holder) : m_holder(holder)
  {
  }

protected:
  virtual void EmitElement(XMLElement * p)
  {
    if (p->tagKey == XMLElement::ET_NODE)
    {
      // store point

      // convert to mercator
      p->lat = MercatorBounds::LatToY(p->lat);
      p->lon = MercatorBounds::LonToX(p->lon);

      m_holder.AddNode(p->id, p->lat, p->lon);
    }
    else if (p->tagKey == XMLElement::ET_WAY)
    {
      // store way
      WayElement way(p->id);

      for (auto const & e: p->childs)
        if (e.tagKey == XMLElement::ET_ND)
          way.nodes.push_back(e.ref);

      if (way.IsValid())
        m_holder.AddWay(p->id, way);
    }
    else if (p->tagKey == XMLElement::ET_RELATION)
    {
      // store relation

      RelationElement relation;
      for (auto const & e: p->childs)
      {
        if (e.tagKey == XMLElement::ET_MEMBER)
        {
          if (e.type == "node")
            relation.nodes.push_back(make_pair(e.ref, e.role));
          else if (e.type == "way")
            relation.ways.push_back(make_pair(e.ref, e.role));
          // we just ignore type == "relation"
        }
        else if (e.tagKey == XMLElement::ET_TAG)
        {
          // relation tags writing as is
          relation.tags.insert(make_pair(e.k, e.v));
        }
      }

      if (relation.IsValid())
        m_holder.AddRelation(p->id, relation);
    }
  }
};
