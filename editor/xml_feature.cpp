#include "editor/xml_feature.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "geometry/latlon.hpp"

namespace
{
constexpr char const * kTimestamp = "timestamp";
constexpr char const * kIndex = "mwm_file_index";
constexpr char const * kUploadTimestamp = "upload_timestamp";
constexpr char const * kUploadStatus = "upload_status";
constexpr char const * kUploadError = "upload_error";
constexpr char const * kHouseNumber = "addr:housenumber";

constexpr char const * kUnknownType = "unknown";
constexpr char const * kNodeType = "node";
constexpr char const * kWayType = "way";
constexpr char const * kRelationType = "relation";

pugi::xml_node FindTag(pugi::xml_document const & document, string const & key)
{
  return document.select_node(("//tag[@k='" + key + "']").data()).node();
}

ms::LatLon GetLatLonFromNode(pugi::xml_node const & node)
{
  ms::LatLon ll;
  if (!strings::to_double(node.attribute("lat").value(), ll.lat))
    MYTHROW(editor::NoLatLon, ("Can't parse lat attribute: " + string(node.attribute("lat").value())));
  if (!strings::to_double(node.attribute("lon").value(), ll.lon))
    MYTHROW(editor::NoLatLon, ("Can't parse lon attribute: " + string(node.attribute("lon").value())));
  return ll;
}

m2::PointD GetMercatorPointFromNode(pugi::xml_node const & node)
{
  m2::PointD p;
  if (!strings::to_double(node.attribute("x").value(), p.x))
    MYTHROW(editor::NoXY, ("Can't parse x attribute: " + string(node.attribute("x").value())));
  if (!strings::to_double(node.attribute("y").value(), p.y))
    MYTHROW(editor::NoXY, ("Can't parse y attribute: " + string(node.attribute("y").value())));
  return p;
}

void ValidateElement(pugi::xml_node const & nodeOrWay)
{
  using editor::XMLFeature;

  if (!nodeOrWay)
    MYTHROW(editor::InvalidXML, ("Document has no valid root element."));

  auto const type = XMLFeature::StringToType(nodeOrWay.name());

  if (type == XMLFeature::Type::Unknown)
    MYTHROW(editor::InvalidXML, ("XMLFeature does not support root tag", nodeOrWay.name()));

  if (type == XMLFeature::Type::Node)
    UNUSED_VALUE(GetLatLonFromNode(nodeOrWay));

  if (!nodeOrWay.attribute(kTimestamp))
    MYTHROW(editor::NoTimestamp, ("Node has no timestamp attribute"));
}
} // namespace

namespace editor
{

char const * const XMLFeature::kDefaultLang =
    StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::kDefaultCode);
char const * const XMLFeature::kIntlLang =
    StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::kInternationalCode);

XMLFeature::XMLFeature(Type const type)
{
  ASSERT_NOT_EQUAL(type, Type::Unknown, ());

  m_document.append_child(TypeToString(type).c_str());
}

XMLFeature::XMLFeature(string const & xml)
{
  m_document.load(xml.data());
  ValidateElement(GetRootNode());
}

XMLFeature::XMLFeature(pugi::xml_document const & xml)
{
  m_document.reset(xml);
  ValidateElement(GetRootNode());
}

XMLFeature::XMLFeature(pugi::xml_node const & xml)
{
  m_document.reset();
  m_document.append_copy(xml);
  ValidateElement(GetRootNode());
}

XMLFeature::XMLFeature(XMLFeature const & feature)
{
  m_document.reset(feature.m_document);
  // Don't validate feature: it should already be validated.
}

bool XMLFeature::operator==(XMLFeature const & other) const
{
  return ToOSMString() == other.ToOSMString();
}

vector<XMLFeature> XMLFeature::FromOSM(string const & osmXml)
{
  pugi::xml_document doc;
  if (doc.load_string(osmXml.data()).status != pugi::status_ok)
    MYTHROW(editor::InvalidXML, ("Not valid XML:", osmXml));

  vector<XMLFeature> features;
  for (auto const n : doc.child("osm").children())
  {
    if (StringToType(n.name()) != Type::Unknown)
      features.emplace_back(n);
  }
  return features;
}

XMLFeature::Type XMLFeature::GetType() const
{
  return StringToType(GetRootNode().name());
}

string XMLFeature::GetTypeString() const
{
  return GetRootNode().name();
}

void XMLFeature::Save(ostream & ost) const
{
  m_document.save(ost, "  ");
}

string XMLFeature::ToOSMString() const
{
  ostringstream ost;
  // Ugly way to wrap into <osm>..</osm> tags.
  // Unfortunately, pugi xml library doesn't allow to insert documents into other documents.
  ost << "<?xml version=\"1.0\"?>" << endl;
  ost << "<osm>" << endl;
  m_document.save(ost, "  ", pugi::format_no_declaration | pugi::format_indent);
  ost << "</osm>" << endl;
  return ost.str();
}

void XMLFeature::ApplyPatch(XMLFeature const & featureWithChanges)
{
  // TODO(mgsergio): Get these alt tags from the config.
  vector<vector<string>> const alternativeTags =
  {
    {"phone", "contact:phone"},
    {"website", "contact:website", "url"},
    {"fax", "contact:fax"},
    {"email", "contact:email"}
  };
  featureWithChanges.ForEachTag([&alternativeTags, this](string const & k, string const & v)
  {
    // Avoid duplication for similar alternative osm tags.
    for (auto const & alt : alternativeTags)
    {
      ASSERT(!alt.empty(), ());
      if (k == alt.front())
      {
        for (auto const & tag : alt)
        {
          // Reuse already existing tag if it's present.
          if (HasTag(tag))
          {
            SetTagValue(tag, v);
            return;
          }
        }
      }
    }
    SetTagValue(k, v);
  });
}

m2::PointD XMLFeature::GetMercatorCenter() const
{
  return MercatorBounds::FromLatLon(GetLatLonFromNode(GetRootNode()));
}

ms::LatLon XMLFeature::GetCenter() const
{
  ASSERT_EQUAL(GetType(), Type::Node, ());
  return GetLatLonFromNode(GetRootNode());
}

void XMLFeature::SetCenter(ms::LatLon const & ll)
{
  ASSERT_EQUAL(GetType(), Type::Node, ());
  SetAttribute("lat", strings::to_string_dac(ll.lat, kLatLonTolerance));
  SetAttribute("lon", strings::to_string_dac(ll.lon, kLatLonTolerance));
}

void XMLFeature::SetCenter(m2::PointD const & mercatorCenter)
{
  SetCenter(MercatorBounds::ToLatLon(mercatorCenter));
}

vector<m2::PointD> XMLFeature::GetGeometry() const
{
  ASSERT_NOT_EQUAL(GetType(), Type::Unknown, ());
  ASSERT_NOT_EQUAL(GetType(), Type::Node, ());
  vector<m2::PointD> geometry;
  for (auto const xCenter : GetRootNode().select_nodes("nd"))
  {
    ASSERT(xCenter.node(), ("no nd attribute."));
    geometry.emplace_back(GetMercatorPointFromNode(xCenter.node()));
  }
  return geometry;
}

string XMLFeature::GetName(string const & lang) const
{
  if (lang == kIntlLang)
    return GetTagValue(kIntlName);
  auto const suffix = (lang == kDefaultLang || lang.empty()) ? "" : ":" + lang;
  return GetTagValue(kDefaultName + suffix);
}

string XMLFeature::GetName(uint8_t const langCode) const
{
  return GetName(StringUtf8Multilang::GetLangByCode(langCode));
}

void XMLFeature::SetName(string const & name)
{
  SetName(kDefaultLang, name);
}

void XMLFeature::SetName(string const & lang, string const & name)
{
  if (lang == kIntlLang)
    SetTagValue(kIntlName, name);
  else
  {
    auto const suffix = (lang == kDefaultLang || lang.empty()) ? "" : ":" + lang;
    SetTagValue(kDefaultName + suffix, name);
  }
}

void XMLFeature::SetName(uint8_t const langCode, string const & name)
{
  SetName(StringUtf8Multilang::GetLangByCode(langCode), name);
}

string XMLFeature::GetHouse() const
{
  return GetTagValue(kHouseNumber);
}

void XMLFeature::SetHouse(string const & house)
{
  SetTagValue(kHouseNumber, house);
}

time_t XMLFeature::GetModificationTime() const
{
  return my::StringToTimestamp(GetRootNode().attribute(kTimestamp).value());
}

void XMLFeature::SetModificationTime(time_t const time)
{
  SetAttribute(kTimestamp, my::TimestampToString(time));
}

uint32_t XMLFeature::GetMWMFeatureIndex() const
{
  // Always cast to uint32_t to avoid warnings on different platforms.
  return static_cast<uint32_t>(GetRootNode().attribute(kIndex).as_uint(0));
}

void XMLFeature::SetMWMFeatureIndex(uint32_t index)
{
  SetAttribute(kIndex, strings::to_string(index));
}

time_t XMLFeature::GetUploadTime() const
{
  return my::StringToTimestamp(GetRootNode().attribute(kUploadTimestamp).value());
}

void XMLFeature::SetUploadTime(time_t const time)
{
  SetAttribute(kUploadTimestamp, my::TimestampToString(time));
}

string XMLFeature::GetUploadStatus() const
{
  return GetRootNode().attribute(kUploadStatus).value();
}

void XMLFeature::SetUploadStatus(string const & status)
{
  SetAttribute(kUploadStatus, status);
}

string XMLFeature::GetUploadError() const
{
  return GetRootNode().attribute(kUploadError).value();
}

void XMLFeature::SetUploadError(string const & error)
{
  SetAttribute(kUploadError, error);
}

bool XMLFeature::HasAnyTags() const
{
  return GetRootNode().child("tag");
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

void XMLFeature::SetTagValue(string const & key, string value)
{
  strings::Trim(value);
  auto tag = FindTag(m_document, key);
  if (!tag)
  {
    tag = GetRootNode().append_child("tag");
    tag.append_attribute("k").set_value(key.data());
    tag.append_attribute("v").set_value(value.data());
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

  node.set_value(value.data());
}

pugi::xml_node const XMLFeature::GetRootNode() const
{
  return m_document.first_child();
}

pugi::xml_node XMLFeature::GetRootNode()
{
  return m_document.first_child();
}

bool XMLFeature::AttachToParentNode(pugi::xml_node parent) const
{
  return !parent.append_copy(GetRootNode()).empty();
}

// static
string XMLFeature::TypeToString(Type type)
{
  switch (type)
  {
  case Type::Unknown: return kUnknownType;
  case Type::Node: return kNodeType;
  case Type::Way: return kWayType;
  case Type::Relation: return kRelationType;
  }
}

// static
XMLFeature::Type XMLFeature::StringToType(string const & type)
{
  if (type == kNodeType)
    return Type::Node;
  if (type == kWayType)
    return Type::Way;
  if (type == kRelationType)
    return Type::Relation;

  return Type::Unknown;
}

string DebugPrint(XMLFeature const & feature)
{
  ostringstream ost;
  feature.Save(ost);
  return ost.str();
}

string DebugPrint(XMLFeature::Type const type)
{
  return XMLFeature::TypeToString(type);
}
} // namespace editor
