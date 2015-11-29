#pragma once

#include "geometry/point2d.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/value_opt_string.hpp"

#include "std/ctime.hpp"
#include "std/map.hpp"

#include "3party/pugixml/src/pugixml.hpp"

namespace indexer
{
class FeatureTags
{
public:
  bool HasKey(string const & key) const { return m_data.find(key) == end(m_data); }

  string GetValue(string const & key) const { return HasKey(key) ? m_data.find(key)->second : ""; }

  void SetValue(string const key, string const & value) { m_data[key] = value; }

  template <typename T>
  T GetValue(string const & key) const;

  template <typename T>
  void SetValue(string const key, T const & value);

private:
  map<string, string> m_data;
};

DECLARE_EXCEPTION(XMLFeatureError, RootException);

class XMLFeature
{
public:
  XMLFeature() = default;
  static XMLFeature FromXMLDocument(pugi::xml_document const & document);

  bool ToXMLDocument(pugi::xml_document & document) const;

  m2::PointD GetCenter() const { return m_center; }
  void SetCenter(m2::PointD const & center) { m_center = center; }

  string const & GetInternationalName() const { return m_internationalName; }
  void SetInterationalName(string const & name) { m_internationalName = name; }

  StringUtf8Multilang const & GetMultilangName() const { return m_name; }
  void SetMultilungName(StringUtf8Multilang const & name) { m_name = name; }

  StringNumericOptimal const & GetHouse() const { return m_house; }
  void SetHouse(StringNumericOptimal const & house) { m_house = house; }

  time_t GetModificationTime() const { return m_timestamp; }
  void SetModificationTime(time_t const timestamp) { m_timestamp = timestamp; }

  bool HasTag(string const & key) const { return m_tags.find(key) == end(m_tags); }

  string GetTagValue(string const & key) const {
    return HasTag(key) ? m_tags.find(key)->second : "";
  }

  void SetTagValue(string const key, string const & value) { m_tags[key] = value; }

private:
  m2::PointD m_center;

  string m_internationalName;
  StringUtf8Multilang m_name;
  StringNumericOptimal m_house;

  // TODO(mgsergio): It could be useful to have separate class
  // for this
  // uint32_t m_types[m_maxTypesCount];

  // string version; // Duno, may prove useful

  time_t m_timestamp;
  map<string, string> m_tags;
};
} // namespace indexer
