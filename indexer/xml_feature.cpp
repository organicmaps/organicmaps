#include "indexer/xml_feature.hpp"

#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "std/cstring.hpp"


namespace
{
string ToString(m2::PointD const & p)
{
  ostringstream out;
  out.precision(20);
  out << p.x << ", " << p.y;
  return out.str();
}

bool FromString(string const & str, m2::PointD & p)
{
  double x, y;
  istringstream sstr(str);
  sstr >> x;
  sstr.get();
  sstr.get();
  sstr >> y;
  p = {x, y};
  return true;
}

void AddTag(string const & key, string const & value, pugi::xml_node & node)
{
  auto tag = node.append_child("tag");
  tag.append_attribute("k") = key.data();
  tag.append_attribute("v") = value.data();
}
}


#include <iostream>
namespace indexer
{
XMLFeature XMLFeature::FromXMLDocument(pugi::xml_document const & document)
{
  XMLFeature feature;

  auto const & node = document.child("node");
  if (!node)
    MYTHROW(XMLFeatureError, ("Document has no node"));

  auto attr = node.attribute("center");
  if (!attr)
    MYTHROW(XMLFeatureError, ("Node has no center attribute"));

  m2::PointD center;
  if (!FromString(attr.value(), center))
    MYTHROW(XMLFeatureError, ("Can't parse center attribute: " + string(attr.value())));
  feature.SetCenter(center);

  if (!(attr = node.attribute("timestamp")))
    MYTHROW(XMLFeatureError, ("Node has no timestamp attribute"));
  feature.SetModificationTime(my::StringToTimestamp(attr.value()));

  for (auto const tag : node.children())
  {
    auto const tagName = tag.attribute("k").value();
    auto const tagValue = tag.attribute("v").value();
    if (strings::StartsWith(tagName, "name::"))
    {
      auto const lang = tagName + strlen("name::");
      feature.m_name.AddString(lang, tagValue);
    }
    else if (strcmp("name", tagName) == 0)
    {
      feature.SetInterationalName(tagValue);
    }
    else if (strcmp("addr::housenumber", tagName) == 0)
    {
      feature.m_house.Set(tagValue);
    }
    else
    {
      feature.m_tags[tagName] = tagValue;
    }
  }

  return feature;
}

bool XMLFeature::ToXMLDocument(pugi::xml_document & document) const
{
  auto node = document.append_child("node");
  node.append_attribute("center") = ToString(GetCenter()).data();
  node.append_attribute("timestamp") = my::TimestampToString(GetModificationTime()).data();

  AddTag("name", GetInternationalName(), node);

  GetMultilangName().ForEachRef([&node](int8_t lang, string const & name) {
      AddTag("name::" + string(StringUtf8Multilang::GetLangByCode(lang)), name, node);
      return true;
    });

  if (!GetHouse().Get().empty())
    AddTag("addr:housenumber", GetHouse().Get(), node);

  for (auto const & tag : m_tags)
    AddTag(tag.first, tag.second, node);

  return true;
}
} // namespace indexer
