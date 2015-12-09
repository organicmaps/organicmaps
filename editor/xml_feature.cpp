#include "editor/xml_feature.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "geometry/mercator.hpp"

#include "std/set.hpp"
#include "std/unordered_set.hpp"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/functional/hash.hpp>

#include "3party/pugixml/src/pugixml.hpp"

namespace
{
auto constexpr kLatLonTolerance = 7;

pugi::xml_node FindTag(pugi::xml_document const & document, string const & key)
{
  return document.select_node(("//tag[@k='" + key + "']").data()).node();
}

m2::PointD PointFromLatLon(pugi::xml_node const node)
{
  double lat, lon;
  if (!strings::to_double(node.attribute("lat").value(), lat))
    MYTHROW(editor::XMLFeatureNoLatLonError,
            ("Can't parse lat attribute: " + string(node.attribute("lat").value())));
 if (!strings::to_double(node.attribute("lon").value(), lon))
   MYTHROW(editor::XMLFeatureNoLatLonError,
           ("Can't parse lon attribute: " + string(node.attribute("lon").value())));
  return MercatorBounds::FromLatLon(lat, lon);
}

void ValidateNode(pugi::xml_node const & node)
{
  if (!node)
    MYTHROW(editor::XMLFeatureNoNodeError, ("Document has no node"));

  try
  {
    PointFromLatLon(node);
  }
  catch(editor::XMLFeatureError)
  {
    throw;
  }

  if (!node.attribute("timestamp"))
    MYTHROW(editor::XMLFeatureNoTimestampError, ("Node has no timestamp attribute"));
}

bool IsConvertable(string const & k, string const & v)
{
  static unordered_set<pair<string, string>, boost::hash<pair<string, string>>> const
      convertableTypePairs =
  {
    { "aeroway", "aerodrome" },
    { "aeroway", "airport" },
    { "amenity", "atm" },
    { "amenity", "bank" },
    { "amenity", "bar" },
    { "amenity", "bbq" },
    { "amenity", "bench" },
    { "amenity", "bicycle_rental" },
    { "amenity", "bureau_de_change" },
    { "amenity", "bus_station" },
    { "amenity", "cafe" },
    { "amenity", "car_rental" },
    { "amenity", "car_sharing" },
    { "amenity", "casino" },
    { "amenity", "cinema" },
    { "amenity", "college" },
    { "amenity", "doctors" },
    { "amenity", "drinking_water" },
    { "amenity", "embassy" },
    { "amenity", "fast_food" },
    { "amenity", "ferry_terminal" },
    { "amenity", "fire_station" },
    { "amenity", "fountain" },
    { "amenity", "fuel" },
    { "amenity", "grave_yard" },
    { "amenity", "hospital" },
    { "amenity", "hunting_stand" },
    { "amenity", "kindergarten" },
    { "amenity", "library" },
    { "amenity", "marketplace" },
    { "amenity", "nightclub" },
    { "amenity", "parking" },
    { "amenity", "pharmacy" },
    { "amenity", "place_of_worship" },
    { "amenity", "police" },
    { "amenity", "post_box" },
    { "amenity", "post_office" },
    { "amenity", "pub" },
    { "amenity", "recycling" },
    { "amenity", "restaurant" },
    { "amenity", "school" },
    { "amenity", "shelter" },
    { "amenity", "taxi" },
    { "amenity", "telephone" },
    { "amenity", "theatre" },
    { "amenity", "toilets" },
    { "amenity", "townhall" },
    { "amenity", "university" },
    { "amenity", "waste_disposal" },
    { "highway", "bus_stop" },
    { "highway", "speed_camera" },
    { "historic", "archaeological_site" },
    { "historic", "castle" },
    { "historic", "memorial" },
    { "historic", "monument" },
    { "historic", "ruins" },
    { "internet", "access" },
    { "internet", "access|wlan" },
    { "landuse", "cemetery" },
    { "leisure", "garden" },
    { "leisure", "pitch" },
    { "leisure", "playground" },
    { "leisure", "sports_centre" },
    { "leisure", "stadium" },
    { "leisure", "swimming_pool" },
    { "natural", "peak" },
    { "natural", "spring" },
    { "natural", "waterfall" },
    { "office", "company" },
    { "office", "estate_agent" },
    { "office", "government" },
    { "office", "lawyer" },
    { "office", "telecommunication" },
    { "place", "farm" },
    { "place", "hamlet" },
    { "place", "village" },
    { "railway", "halt" },
    { "railway", "station" },
    { "railway", "subway_entrance" },
    { "railway", "tram_stop" },
    { "shop", "alcohol" },
    { "shop", "bakery" },
    { "shop", "beauty" },
    { "shop", "beverages" },
    { "shop", "bicycle" },
    { "shop", "books" },
    { "shop", "butcher" },
    { "shop", "car" },
    { "shop", "car_repair" },
    { "shop", "chemist" },
    { "shop", "clothes" },
    { "shop", "computer" },
    { "shop", "confectionery" },
    { "shop", "convenience" },
    { "shop", "department_store" },
    { "shop", "doityourself" },
    { "shop", "electronics" },
    { "shop", "florist" },
    { "shop", "furniture" },
    { "shop", "garden_centre" },
    { "shop", "gift" },
    { "shop", "greengrocer" },
    { "shop", "hairdresser" },
    { "shop", "hardware" },
    { "shop", "jewelry" },
    { "shop", "kiosk" },
    { "shop", "laundry" },
    { "shop", "mall" },
    { "shop", "mobile_phone" },
    { "shop", "optician" },
    { "shop", "shoes" },
    { "shop", "sports" },
    { "shop", "supermarket" },
    { "shop", "toys" },
    { "tourism", "alpine_hut" },
    { "tourism", "artwork" },
    { "tourism", "attraction" },
    { "tourism", "camp_site" },
    { "tourism", "caravan_site" },
    { "tourism", "guest_house" },
    { "tourism", "hostel" },
    { "tourism", "hotel" },
    { "tourism", "information" },
    { "tourism", "motel" },
    { "tourism", "museum" },
    { "tourism", "picnic_site" },
    { "tourism", "viewpoint" },
    { "waterway", "waterfall" }
  };

  return convertableTypePairs.find(make_pair(k, v)) != end(convertableTypePairs);
}

pair<string, string> SplitMapsmeType(string const & type)
{
  vector<string> parts;
  boost::split(parts, type, boost::is_any_of("|"));
  // Current implementations supports only types with one pipe
  ASSERT(parts.size() == 2, ("Too many parts in type: " + type));
  return make_pair(parts[0], parts[1]);
}
} // namespace

namespace editor
{
XMLFeature::XMLFeature()
{
  m_document.append_child("node");
}

XMLFeature::XMLFeature(string const & xml)
{
  m_document.load(xml.data());
  ValidateNode(GetRootNode());
}

XMLFeature::XMLFeature(pugi::xml_document const & xml)
{
  m_document.reset(xml);
  ValidateNode(GetRootNode());
}

XMLFeature::XMLFeature(pugi::xml_node const & xml)
{
  m_document.reset();
  m_document.append_copy(xml);
  ValidateNode(GetRootNode());
}

void XMLFeature::Save(ostream & ost) const
{
  m_document.save(ost, "  ", pugi::format_indent_attributes);
}

string XMLFeature::GetType() const
{
  for (auto const tag : GetRootNode().select_nodes("tag"))
  {
    string const key = tag.node().attribute("k").value();
    string const val = tag.node().attribute("v").value();
    // Handle only the first appropriate tag in first version
    if (IsConvertable(key, val))
      return key + '|' + val;
  }

  return "";
}

void XMLFeature::SetType(string const & type)
{
  auto const p = SplitMapsmeType(type);
  if (IsConvertable(p.first, p.second))
    SetTagValue(p.first, p.second);
}

m2::PointD XMLFeature::GetCenter() const
{
  return PointFromLatLon(GetRootNode());
}

void XMLFeature::SetCenter(m2::PointD const & mercatorCenter)
{
  SetAttribute("lat", strings::to_string_dac(MercatorBounds::YToLat(mercatorCenter.y),
                                             kLatLonTolerance));
  SetAttribute("lon", strings::to_string_dac(MercatorBounds::XToLon(mercatorCenter.x),
                                             kLatLonTolerance));
}

string XMLFeature::GetName(string const & lang) const
{
  auto const suffix = lang == "default" || lang.empty() ? "" : ":" + lang;
  return GetTagValue("name" + suffix);
}

string XMLFeature::GetName(uint8_t const langCode) const
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

string XMLFeature::GetHouse() const
{
  return GetTagValue("addr:housenumber");
}

void XMLFeature::SetHouse(string const & house)
{
  SetTagValue("addr:housenumber", house);
}

time_t XMLFeature::GetModificationTime() const
{
  auto const node = GetRootNode();
  return my::StringToTimestamp(node.attribute("timestamp").value());
}

void XMLFeature::SetModificationTime(time_t const time)
{
  SetAttribute("timestamp", my::TimestampToString(time));
}

bool XMLFeature::HasTag(string const & key) const
{
  return FindTag(m_document, key);
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
  auto const tag = FindTag(m_document, key);
  return tag.attribute("v").value();
}

void XMLFeature::SetTagValue(string const & key, string const value)
{
  auto tag = FindTag(m_document, key);
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

pugi::xml_node const XMLFeature::GetRootNode() const
{
  return m_document.child("node");
}

pugi::xml_node XMLFeature::GetRootNode()
{
  return m_document.child("node");
}
} // namespace editor
