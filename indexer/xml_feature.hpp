#pragma once

#include "geometry/point2d.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/value_opt_string.hpp"

#include "std/ctime.hpp"
#include "std/map.hpp"
#include "std/unique_ptr.hpp"


namespace pugi
{
class xml_document;
}

namespace indexer
{

DECLARE_EXCEPTION(XMLFeatureError, RootException);

class XMLFeature
{
public:
  XMLFeature();
  XMLFeature(string const & xml);

  pugi::xml_document const & GetXMLDocument() const;

  m2::PointD GetCenter() const;

  string const GetName(string const & lang = "") const;
  string const GetName(uint8_t const langCode) const;

  string const GetHouse() const;

  time_t GetModificationTime() const;

  bool HasTag(string const & key) const;

  string GetTagValue(string const & key) const;

private:
  unique_ptr<pugi::xml_document> m_documentPtr;
};
} // namespace indexer
