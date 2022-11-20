#pragma once

#include "kml/types.hpp"

#include "coding/parse_xml.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/exception.hpp"
#include "base/stl_helpers.hpp"

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

  void Write(FileData const & fileData);

private:
  WriterWrapper m_writer;
};

class SerializerKml
{
public:
  DECLARE_EXCEPTION(SerializeException, RootException);

  explicit SerializerKml(FileData const & fileData)
    : m_fileData(fileData)
  {}

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    KmlWriter kmlWriter(sink);
    kmlWriter.Write(m_fileData);
  }

private:
  FileData const & m_fileData;
};

class KmlParser
{
public:
  explicit KmlParser(FileData & data);
  bool Push(std::string const & name);
  void AddAttr(std::string const & attr, std::string const & value);
  bool IsValidAttribute(std::string const & type, std::string const & value,
                        std::string const & attrInLowerCase) const;
  std::string const & GetTagFromEnd(size_t n) const;
  void Pop(std::string const & tag);
  void CharData(std::string value);

  static kml::TrackLayer GetDefaultTrackLayer();

private:
  enum GeometryType
  {
    GEOMETRY_TYPE_UNKNOWN,
    GEOMETRY_TYPE_POINT,
    GEOMETRY_TYPE_LINE
  };

  void ResetPoint();
  void SetOrigin(std::string const & s);
  void ParseLineCoordinates(std::string const & s, char const * blockSeparator,
                            char const * coordSeparator);
  bool MakeValid();
  void ParseColor(std::string const &value);
  bool GetColorForStyle(std::string const & styleUrl, uint32_t & color) const;
  double GetTrackWidthForStyle(std::string const & styleUrl) const;

  FileData & m_data;
  CategoryData m_compilationData;
  CategoryData * m_categoryData;  // never null

  std::vector<std::string> m_tags;
  GeometryType m_geometryType;
  std::vector<geometry::PointWithAltitude> m_pointsWithAltitudes;
  uint32_t m_color;

  std::string m_styleId;
  std::string m_mapStyleId;
  std::string m_styleUrlKey;
  std::map<std::string, uint32_t> m_styleUrl2Color;
  std::map<std::string, double> m_styleUrl2Width;
  std::map<std::string, std::string> m_mapStyle2Style;

  int8_t m_attrCode;
  std::string m_attrId;
  std::string m_attrKey;

  LocalizableString m_name;
  LocalizableString m_description;
  PredefinedColor m_predefinedColor;
  Timestamp m_timestamp;
  m2::PointD m_org;
  uint8_t m_viewportScale;
  std::vector<uint32_t> m_featureTypes;
  LocalizableString m_customName;
  std::vector<LocalId> m_boundTracks;
  LocalId m_localId;
  BookmarkIcon m_icon;
  std::vector<TrackLayer> m_trackLayers;
  bool m_visible;
  std::string m_nearestToponym;
  std::vector<std::string> m_nearestToponyms;
  int m_minZoom = 1;
  kml::Properties m_properties;
  std::vector<CompilationId> m_compilations;
  double m_trackWidth;
};

class DeserializerKml
{
public:
  DECLARE_EXCEPTION(DeserializeException, RootException);

  explicit DeserializerKml(FileData & fileData);

  template <typename ReaderType>
  void Deserialize(ReaderType const & reader)
  {
    NonOwningReaderSource src(reader);
    KmlParser parser(m_fileData);
    if (!ParseXML(src, parser, true))
    {
      // Print corrupted KML file for debug and restore purposes.
      std::string kmlText;
      reader.ReadAsString(kmlText);
      if (!kmlText.empty() && kmlText[0] == '<')
        LOG(LWARNING, (kmlText));
      MYTHROW(DeserializeException, ("Could not parse KML."));
    }
  }

private:
  FileData & m_fileData;
};
}  // namespace kml
