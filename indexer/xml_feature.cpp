#include "indexer/xml_feature.hpp"

#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "std/cstring.hpp"

#include "3party/pugixml/src/pugixml.hpp"


namespace
{
string ToString(m2::PointD const & p)
{
  ostringstream out;
  out.precision(7);
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

pugi::xpath_node FindTag(pugi::xml_document const & document, string const & key)
{
  return document.select_node(("//tag[@k='" + key + "']").data());
}
} // namespace


namespace indexer
{
XMLFeature::XMLFeature(): m_documentPtr(new pugi::xml_document) {}

XMLFeature::XMLFeature(string const & xml):
    XMLFeature()
{
  m_documentPtr->load(xml.data());

  auto const node = m_documentPtr->child("node");
  if (!node)
    MYTHROW(XMLFeatureError, ("Document has no node"));

  auto attr = node.attribute("center");
  if (!attr)
    MYTHROW(XMLFeatureError, ("Node has no center attribute"));

  m2::PointD center;
  if (!FromString(attr.value(), center))
    MYTHROW(XMLFeatureError, ("Can't parse center attribute: " + string(attr.value())));

  if (!(attr = node.attribute("timestamp")))
    MYTHROW(XMLFeatureError, ("Node has no timestamp attribute"));
}

pugi::xml_document const & XMLFeature::GetXMLDocument() const
{
  return *m_documentPtr;
}

m2::PointD XMLFeature::GetCenter() const
{
  auto const node = m_documentPtr->child("node");
  m2::PointD center;
  FromString(node.attribute("center").value(), center);
  return center;
}

string const XMLFeature::GetName(string const & lang) const
{
  auto const suffix = lang == "default" || lang.empty() ? "" : "::" + lang;
  return GetTagValue("name" + suffix);
}

string const XMLFeature::GetName(uint8_t const langCode) const
{
  return GetName(StringUtf8Multilang::GetLangByCode(langCode));
}

string const XMLFeature::GetHouse() const
{
  return GetTagValue("addr:housenumber");
}

time_t XMLFeature::GetModificationTime() const
{
  auto const node = m_documentPtr->child("node");
  return my::StringToTimestamp(node.attribute("timestamp").value());
}

bool XMLFeature::HasTag(string const & key) const
{
  return FindTag(*m_documentPtr, key);
}

string XMLFeature::GetTagValue(string const & key) const
{
  auto const tag = FindTag(*m_documentPtr, key);
  return tag.node().attribute("v").value();
}

// bool XMLFeature::ToXMLDocument(pugi::xml_document & document) const
// {
//   auto node = document.append_child("node");
//   node.append_attribute("center") = ToString(GetCenter()).data();
//   node.append_attribute("timestamp") = my::TimestampToString(GetModificationTime()).data();

//   AddTag("name", GetInternationalName(), node);

//   GetMultilangName().ForEachRef([&node](int8_t lang, string const & name) {
//       AddTag("name::" + string(StringUtf8Multilang::GetLangByCode(lang)), name, node);
//       return true;
//     });

//   if (!GetHouse().Get().empty())
//     AddTag("addr:housenumber", GetHouse().Get(), node);

//   for (auto const & tag : m_tags)
//     AddTag(tag.first, tag.second, node);

//   return true;
// }


} // namespace indexer
