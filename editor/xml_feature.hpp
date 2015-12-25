#pragma once

#include "geometry/point2d.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/string_utils.hpp"

#include "std/ctime.hpp"
#include "std/iostream.hpp"

#include "3party/pugixml/src/pugixml.hpp"

namespace editor
{
DECLARE_EXCEPTION(XMLFeatureError, RootException);
DECLARE_EXCEPTION(XMLFeatureNoNodeError, XMLFeatureError);
DECLARE_EXCEPTION(XMLFeatureNoLatLonError, XMLFeatureError);
DECLARE_EXCEPTION(XMLFeatureNoTimestampError, XMLFeatureError);
DECLARE_EXCEPTION(XMLFeatureNoHeaderError, XMLFeatureError);

class XMLFeature
{
  static constexpr char const * kDefaultName = "name";
  static constexpr char const * kLocalName = "name:";
  static char const * const kDefaultLang;

public:
  XMLFeature();
  XMLFeature(string const & xml);
  XMLFeature(pugi::xml_document const & xml);
  XMLFeature(pugi::xml_node const & xml);
  XMLFeature(XMLFeature const & feature) : XMLFeature(feature.m_document) {}
  void Save(ostream & ost) const;

  m2::PointD GetCenter() const;
  void SetCenter(m2::PointD const & mercatorCenter);

  string GetType() const;
  void SetType(string const & type);

  string GetName(string const & lang) const;
  string GetName(uint8_t const langCode = StringUtf8Multilang::DEFAULT_CODE) const;

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
  uint32_t GetOffset() const;
  void SetOffset(uint32_t featureOffset);

  /// @returns my::INVALID_TIME_STAMP if there were no any upload attempt.
  time_t GetUploadTime() const;
  void SetUploadTime(time_t const time);

  string GetUploadStatus() const;
  void SetUploadStatus(string const & status);

  string GetUploadError() const;
  void SetUploadError(string const & error);
  //@}

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
  void SetTagValue(string const & key, string const value);

  string GetAttribute(string const & key) const;
  void SetAttribute(string const & key, string const & value);

  bool AttachToParentNode(pugi::xml_node parent) const;

private:
  pugi::xml_node const GetRootNode() const;
  pugi::xml_node GetRootNode();

  pugi::xml_document m_document;
};
} // namespace editor
