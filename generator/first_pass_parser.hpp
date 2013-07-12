#pragma once

#include "xml_element.hpp"
#include "osm_decl.hpp"

#include "../indexer/mercator.hpp"

#include "../base/string_utils.hpp"


template <class THolder>
class FirstPassParser : public BaseOSMParser
{
  THolder & m_holder;

public:
  FirstPassParser(THolder & holder) : m_holder(holder)
  {
    static char const * tags[] = { "osm", "node", "way", "relation" };
    SetTags(tags);
  }

protected:
  virtual void EmitElement(XMLElement * p)
  {
    uint64_t id;
    VERIFY ( strings::to_uint64(p->attrs["id"], id), ("Unknown element with invalid id : ", p->attrs["id"]) );

    if (p->name == "node")
    {
      // store point

      double lat, lng;
      VERIFY ( strings::to_double(p->attrs["lat"], lat), ("Bad node lat : ", p->attrs["lat"]) );
      VERIFY ( strings::to_double(p->attrs["lon"], lng), ("Bad node lon : ", p->attrs["lon"]) );

      // convert to mercator
      lat = MercatorBounds::LatToY(lat);
      lng = MercatorBounds::LonToX(lng);

      m_holder.AddNode(id, lat, lng);
    }
    else if (p->name == "way")
    {
      // store way
      WayElement e(id);

      for (size_t i = 0; i < p->childs.size(); ++i)
      {
        if (p->childs[i].name == "nd")
        {
          uint64_t ref;
          VERIFY ( strings::to_uint64(p->childs[i].attrs["ref"], ref), ("Bad node ref in way : ", p->childs[i].attrs["ref"]) );
          e.nodes.push_back(ref);
        }
      }

      if (e.IsValid())
        m_holder.AddWay(id, e);
    }
    else if (p->name == "relation")
    {
      // store relation

      RelationElement e;
      for (size_t i = 0; i < p->childs.size(); ++i)
      {
        if (p->childs[i].name == "member")
        {
          uint64_t ref;
          VERIFY ( strings::to_uint64(p->childs[i].attrs["ref"], ref), ("Bad ref in relation : ", p->childs[i].attrs["ref"]) );

          string const & type = p->childs[i].attrs["type"];
          string const & role = p->childs[i].attrs["role"];
          if (type == "node")
            e.nodes.push_back(make_pair(ref, role));
          else if (type == "way")
            e.ways.push_back(make_pair(ref, role));
          // we just ignore type == "relation"
        }
        else if (p->childs[i].name == "tag")
        {
          // relation tags writing as is
          e.tags.insert(make_pair(p->childs[i].attrs["k"], p->childs[i].attrs["v"]));
        }
      }

      if (e.IsValid())
        m_holder.AddRelation(id, e);
    }
  }
};
