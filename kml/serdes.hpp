#pragma once

#include "kml/types.hpp"

#include "coding/parse_xml.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/exception.hpp"
#include "base/stl_add.hpp"

#include <chrono>
#include <string>

namespace kml
{
class KmlWriter
{
public:
  DECLARE_EXCEPTION(WriteKmlException, RootException);

  class WriterWrapper
  {
  public:
    explicit WriterWrapper(Writer & writer)
      : m_writer(writer)
    {}
    WriterWrapper & operator<<(std::string const & str);
  private:
    Writer & m_writer;
  };

  explicit KmlWriter(Writer & writer)
    : m_writer(writer)
  {}

  void Write(CategoryData const & categoryData);

private:
  WriterWrapper m_writer;
};

class SerializerKml
{
public:
  explicit SerializerKml(CategoryData const & categoryData)
    : m_categoryData(categoryData)
  {}

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    KmlWriter kmlWriter(sink);
    kmlWriter.Write(m_categoryData);
  }

private:
  CategoryData const & m_categoryData;
};

class KmlParser
{
public:
  explicit KmlParser(CategoryData & data);
  bool Push(std::string const & name);
  void AddAttr(std::string const & attr, std::string const & value);
  bool IsValidAttribute(std::string const & type, std::string const & value,
                        std::string const & attrInLowerCase) const;
  std::string const & GetTagFromEnd(size_t n) const;
  void Pop(std::string const & tag);
  void CharData(std::string value);

private:
  enum GeometryType
  {
    GEOMETRY_TYPE_UNKNOWN,
    GEOMETRY_TYPE_POINT,
    GEOMETRY_TYPE_LINE
  };

  void Reset();
  bool ParsePoint(std::string const & s, char const * delim, m2::PointD & pt);
  void SetOrigin(std::string const & s);
  void ParseLineCoordinates(std::string const & s, char const * blockSeparator,
                            char const * coordSeparator);
  bool MakeValid();
  void ParseColor(std::string const &value);
  bool GetColorForStyle(std::string const & styleUrl, uint32_t & color);

  CategoryData & m_data;

  std::vector<std::string> m_tags;
  GeometryType m_geometryType;
  std::vector<m2::PointD> m_points;
  uint32_t m_color;

  std::string m_styleId;
  std::string m_mapStyleId;
  std::string m_styleUrlKey;
  std::map<std::string, uint32_t> m_styleUrl2Color;
  std::map<std::string, std::string> m_mapStyle2Style;

  std::string m_name;
  std::string m_description;
  PredefinedColor m_predefinedColor;
  Timestamp m_timestamp;
  m2::PointD m_org;
  uint8_t m_viewportScale;
};

class DeserializerKml
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit DeserializerKml(CategoryData & categoryData);

  template <typename ReaderType>
  void Deserialize(ReaderType const & reader)
  {
    NonOwningReaderSource src(reader);
    KmlParser parser(m_categoryData);
    if (!ParseXML(src, parser, true))
      MYTHROW(DeserializeException, ("Could not parse KML."));
  }

private:
  CategoryData & m_categoryData;
};
}  // namespace kml
