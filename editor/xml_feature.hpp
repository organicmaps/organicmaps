#pragma once

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/string_utils.hpp"

#include "std/ctime.hpp"
#include "std/iostream.hpp"
#include "std/vector.hpp"

#include "3party/pugixml/src/pugixml.hpp"

class FeatureType;

namespace editor
{
DECLARE_EXCEPTION(XMLFeatureError, RootException);
DECLARE_EXCEPTION(InvalidXML, XMLFeatureError);
DECLARE_EXCEPTION(NoLatLon, XMLFeatureError);
DECLARE_EXCEPTION(NoXY, XMLFeatureError);
DECLARE_EXCEPTION(NoTimestamp, XMLFeatureError);
DECLARE_EXCEPTION(NoHeader, XMLFeatureError);

class XMLFeature
{
  static constexpr char const * kDefaultName = "name";
  static constexpr char const * kIntlName = "int_name";
  static constexpr char const * kLocalName = "name:";
  static char const * const kDefaultLang;
  static char const * const kIntlLang;

public:
  // Used in point to string serialization.
  static constexpr int kLatLonTolerance = 7;

  enum class Type
  {
    Unknown,
    Node,
    Way,
    Relation
  };

  /// Creates empty node or way.
  XMLFeature(Type const type);
  XMLFeature(string const & xml);
  XMLFeature(pugi::xml_document const & xml);
  XMLFeature(pugi::xml_node const & xml);
  XMLFeature(XMLFeature const & feature);
  // TODO: It should make "deep" compare instead of converting to strings.
  // Strings comparison does not work if tags order is different but tags are equal.
  bool operator==(XMLFeature const & other) const;
  /// @returns nodes, ways and relations from osmXml. Vector can be empty.
  static vector<XMLFeature> FromOSM(string const & osmXml);

  void Save(ostream & ost) const;
  string ToOSMString() const;

  /// Tags from featureWithChanges are applied to this(osm) feature.
  void ApplyPatch(XMLFeature const & featureWithChanges);

  Type GetType() const;
  string GetTypeString() const;

  m2::PointD GetMercatorCenter() const;
  ms::LatLon GetCenter() const;
  void SetCenter(ms::LatLon const & ll);
  void SetCenter(m2::PointD const & mercatorCenter);

  vector<m2::PointD> GetGeometry() const;

  /// Sets geometry in mercator to match against FeatureType's geometry in mwm
  /// when megrating to a new mwm build.
  /// Geometry points are now stored in <nd x="..." y="..." /> nodes like in osm <way>.
  /// But they are not the same as osm's. I.e. osm's one stores reference to a <node>
  /// with it's own data and lat, lon. Here we store only cooridanes in mercator.
  template <typename TIterator>
  void SetGeometry(TIterator begin, TIterator end)
  {
    ASSERT_NOT_EQUAL(GetType(), Type::Unknown, ());
    ASSERT_NOT_EQUAL(GetType(), Type::Node, ());

    for (; begin != end; ++begin)
    {
      auto nd = GetRootNode().append_child("nd");
      nd.append_attribute("x") = strings::to_string_dac(begin->x, kLatLonTolerance).data();
      nd.append_attribute("y") = strings::to_string_dac(begin->y, kLatLonTolerance).data();
    }
  }

  template <typename TCollection>
  void SetGeometry(TCollection const & geometry)
  {
    SetGeometry(begin(geometry), end(geometry));
  }

  string GetName(string const & lang) const;
  string GetName(uint8_t const langCode = StringUtf8Multilang::kDefaultCode) const;

  template <typename TFunc>
  void ForEachName(TFunc && func) const
  {
    static auto const kPrefixLen = strlen(kLocalName);
    auto const tags = GetRootNode().select_nodes("tag");
    for (auto const & tag : tags)
    {
      string const & key = tag.node().attribute("k").value();

      if (strings::StartsWith(key, kLocalName))
        func(key.substr(kPrefixLen), tag.node().attribute("v").value());
      else if (key == kDefaultName)
        func(kDefaultLang, tag.node().attribute("v").value());
    }
  }

  void SetName(string const & name);
  void SetName(string const & lang, string const & name);
  void SetName(uint8_t const langCode, string const & name);

  string GetHouse() const;
  void SetHouse(string const & house);

  /// Our and OSM modification time are equal.
  time_t GetModificationTime() const;
  void SetModificationTime(time_t const time);

  /// @name XML storage format helpers.
  //@{
  uint32_t GetMWMFeatureIndex() const;
  void SetMWMFeatureIndex(uint32_t index);

  /// @returns base::INVALID_TIME_STAMP if there were no any upload attempt.
  time_t GetUploadTime() const;
  void SetUploadTime(time_t const time);

  string GetUploadStatus() const;
  void SetUploadStatus(string const & status);

  string GetUploadError() const;
  void SetUploadError(string const & error);
  //@}

  bool HasAnyTags() const;
  bool HasTag(string const & key) const;
  bool HasAttribute(string const & key) const;
  bool HasKey(string const & key) const;

  template <typename TFunc>
  void ForEachTag(TFunc && func) const
  {
    for (auto const & tag : GetRootNode().select_nodes("tag"))
      func(tag.node().attribute("k").value(), tag.node().attribute("v").value());
  }

  string GetTagValue(string const & key) const;
  void SetTagValue(string const & key, string value);

  string GetAttribute(string const & key) const;
  void SetAttribute(string const & key, string const & value);

  bool AttachToParentNode(pugi::xml_node parent) const;

  static string TypeToString(Type type);
  static Type StringToType(string const & type);

private:
  pugi::xml_node const GetRootNode() const;
  pugi::xml_node GetRootNode();

  pugi::xml_document m_document;
};

/// Rewrites all but geometry and types.
/// Should be applied to existing features only (in mwm files).
void ApplyPatch(XMLFeature const & xml, FeatureType & feature);

/// @param serializeType if false, types are not serialized.
/// Useful for applying modifications to existing OSM features, to avoid issues when someone
/// has changed a type in OSM, but our users uploaded invalid outdated type after modifying feature.
XMLFeature ToXML(FeatureType & feature, bool serializeType);

/// Creates new feature, including geometry and types.
/// @Note: only nodes (points) are supported at the moment.
bool FromXML(XMLFeature const & xml, FeatureType & feature);

string DebugPrint(XMLFeature const & feature);
string DebugPrint(XMLFeature::Type const type);
} // namespace editor
