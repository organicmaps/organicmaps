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
  out << fixed;
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

pugi::xml_node FindTag(pugi::xml_document const & document, string const & key)
{
  return document.select_node(("//tag[@k='" + key + "']").data()).node();
}
} // namespace


namespace indexer
{
XMLFeature::XMLFeature(): m_documentPtr(new pugi::xml_document)
{
  m_documentPtr->append_child("node");
}

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

XMLFeature::XMLFeature(pugi::xml_document const & xml):
    XMLFeature()
{
  m_documentPtr->reset(xml);
}

void XMLFeature::Save(ostream & ost) const
{
  m_documentPtr->save(ost, "\t", pugi::format_indent_attributes);
}

m2::PointD XMLFeature::GetCenter() const
{
  auto const node = m_documentPtr->child("node");
  m2::PointD center;
  FromString(node.attribute("center").value(), center);
  return center;
}

void XMLFeature::SetCenter(m2::PointD const & center)
{
  SetAttribute("center", ToString(center));
}

string const XMLFeature::GetName(string const & lang) const
{
  auto const suffix = lang == "default" || lang.empty() ? "" : ":" + lang;
  return GetTagValue("name" + suffix);
}

string const XMLFeature::GetName(uint8_t const langCode) const
{
  return GetName(StringUtf8Multilang::GetLangByCode(langCode));
}

void XMLFeature::SetName(string const & name)
{
  SetName("default", name);
}

void XMLFeature::SetName(string const & lang, string const & name)
{
  auto const suffix = lang == "default" || lang.empty() ? "" : ":" + lang;
  SetTagValue("name" + suffix, name);
}

void XMLFeature::SetName(uint8_t const langCode, string const & name)
{
  SetName(StringUtf8Multilang::GetLangByCode(langCode), name);
}

string const XMLFeature::GetHouse() const
{
  return GetTagValue("addr:housenumber");
}

void XMLFeature::SetHouse(string const & house)
{
  SetTagValue("addr:housenumber", house);
}

time_t XMLFeature::GetModificationTime() const
{
  auto const node = m_documentPtr->child("node");
  return my::StringToTimestamp(node.attribute("timestamp").value());
}

void XMLFeature::SetModificationTime(time_t const time)
{
  SetAttribute("timestamp", my::TimestampToString(time));
}

bool XMLFeature::HasTag(string const & key) const
{
  return FindTag(*m_documentPtr, key);
}

bool XMLFeature::HasAttribute(string const & key) const
{
  return GetRootNode().attribute(key.data());
}

bool XMLFeature::HasKey(string const & key) const
{
  return HasTag(key) || HasAttribute(key);
}

string XMLFeature::GetTagValue(string const & key) const
{
  auto const tag = FindTag(*m_documentPtr, key);
  return tag.attribute("v").value();
}

void XMLFeature::SetTagValue(string const & key, string const value)
{
  auto tag = FindTag(*m_documentPtr, key);
  if (!tag)
  {
    tag = GetRootNode().append_child("tag");
    tag.append_attribute("k") = key.data();
    tag.append_attribute("v") = value.data();
  }
  else
  {
    tag.attribute("v") = value.data();
  }
}

string XMLFeature::GetAttribute(string const & key) const
{
  return GetRootNode().attribute(key.data()).value();
}

void XMLFeature::SetAttribute(string const & key, string const & value)
{
  auto node = HasAttribute(key)
      ? GetRootNode().attribute(key.data())
      : GetRootNode().append_attribute(key.data());

  node = value.data();
}

pugi::xml_node XMLFeature::GetRootNode() const
{
  return m_documentPtr->child("node");
}
} // namespace indexer
