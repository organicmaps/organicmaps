#pragma once

#include "kml/serdes_common.hpp"
#include "kml/types.hpp"

#include "coding/parse_xml.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/point2d.hpp"

#include "base/exception.hpp"

#include <string>

namespace kml
{
class KmlWriter
{
public:
  DECLARE_EXCEPTION(WriteKmlException, RootException);

  explicit KmlWriter(Writer & writer) : m_writer(writer) {}

  void Write(FileData const & fileData);

private:
  Writer & m_writer;
};

class SerializerKml
{
public:
  DECLARE_EXCEPTION(SerializeException, RootException);

  explicit SerializerKml(FileData const & fileData) : m_fileData(fileData) {}

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

  /// @name Parser callback functions.
  /// @{
  bool Push(std::string name);
  void AddAttr(std::string attr, std::string value);
  void Pop(std::string_view tag);
  void CharData(std::string & value);
  /// @}

  bool IsValidAttribute(std::string_view type, std::string const & value, std::string const & attrInLowerCase) const;

  static kml::TrackLayer GetDefaultTrackLayer();

private:
  std::string const & GetTagFromEnd(size_t n) const;
  bool IsProcessTrackTag() const;
  bool IsProcessTrackCoord() const;

  enum GeometryType
  {
    GEOMETRY_TYPE_UNKNOWN,
    GEOMETRY_TYPE_POINT,
    GEOMETRY_TYPE_LINE
  };

  void ResetPoint();
  void SetOrigin(std::string const & s);
  static void ParseAndAddPoints(MultiGeometry::LineT & line, std::string_view s, char const * blockSeparator,
                                char const * coordSeparator);
  void ParseLineString(std::string const & s);

  bool MakeValid();
  void ParseColor(std::string const & value);
  bool GetColorForStyle(std::string const & styleUrl, uint32_t & color) const;
  double GetTrackWidthForStyle(std::string const & styleUrl) const;

  FileData & m_data;
  CategoryData m_compilationData;
  CategoryData * m_categoryData;  // never null

  std::vector<std::string> m_tags;
  GeometryType m_geometryType;

  MultiGeometry m_geometry;
  std::map<size_t, std::set<size_t>> m_skipTimes;
  size_t m_lastTrackPointsCount;

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
