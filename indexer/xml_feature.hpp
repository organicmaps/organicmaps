#pragma once

#include "geometry/point2d.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/value_opt_string.hpp"

#include "std/ctime.hpp"
#include "std/iostream.hpp"
#include "std/map.hpp"
#include "std/unique_ptr.hpp"

namespace pugi
{
class xml_document;
class xml_node;
}

namespace indexer
{

DECLARE_EXCEPTION(XMLFeatureError, RootException);

class XMLFeature
{
public:
  XMLFeature();
  XMLFeature(string const & xml);
  XMLFeature(pugi::xml_document const & xml);

  void Save(ostream & ost) const;

  m2::PointD GetCenter() const;
  void SetCenter(m2::PointD const & center);

  string const GetName(string const & lang = "") const;
  string const GetName(uint8_t const langCode) const;

  void SetName(string const & name);
  void SetName(string const & lang, string const & name);
  void SetName(uint8_t const langCode, string const & name);

  string const GetHouse() const;
  void SetHouse(string const & house);

  time_t GetModificationTime() const;
  void SetModificationTime(time_t const time);

  bool HasTag(string const & key) const;
  bool HasAttribute(string const & key) const;
  bool HasKey(string const & key) const;

  string GetTagValue(string const & key) const;
  void SetTagValue(string const & key, string const value);

  string GetAttribute(string const & key) const;
  void SetAttribute(string const & key, string const & value);

private:
  pugi::xml_node GetRootNode() const;

  unique_ptr<pugi::xml_document> m_documentPtr;
};
} // namespace indexer
