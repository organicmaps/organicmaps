#pragma once

#include "kml/types.hpp"
#include "kml/serdes_common.hpp"

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

  class WriterWrapper
  {
  public:
    explicit WriterWrapper(Writer & writer)
      : m_writer(writer)
    {}
    WriterWrapper & operator<<(std::string_view str);
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

  /// @name Parser callback functions.
  /// @{
  bool Push(std::string name);
  void AddAttr(std::string attr, std::string value);
  void Pop(std::string_view tag);
  void CharData(std::string & value);
  /// @}

  bool IsValidAttribute(std::string_view type, std::string const & value,
                        std::string const & attrInLowerCase) const;

  static kml::TrackLayer GetDefaultTrackLayer();

private:
  std::string const & GetTagFromEnd(size_t n) const;
  bool IsProcessTrackTag() const;

  enum GeometryType
  {
    GEOMETRY_TYPE_UNKNOWN,
    GEOMETRY_TYPE_POINT,
    GEOMETRY_TYPE_LINE
  };

  void ResetPoint();
  void SetOrigin(std::string const & s);
  static void ParseAndAddPoints(MultiGeometry::LineT & line, std::string_view s,
                                char const * blockSeparator, char const * coordSeparator);
  void ParseLineString(std::string const & s);

  bool MakeValid();
  void ParseColor(std::string const & value);

  FileData & m_data;
  CategoryData m_compilationData;
  CategoryData * m_categoryData;  // never null

  std::vector<std::string> m_tags;
  GeometryType m_geometryType;
  MultiGeometry m_geometry;

  std::string m_styleId;
  std::string m_mapStyleId;
  std::string m_styleUrlKey;

  struct StyleParams
  {
    static uint32_t constexpr kInvalidColor = uint32_t(-1);
    static double constexpr kDefaultWidth = 5.0;

    uint32_t color = kInvalidColor;
    double width = kDefaultWidth;
    std::string iconPath;

    void Invalidate()
    {
      color = kInvalidColor;
      width = kDefaultWidth;
      iconPath.clear();
    }
    uint32_t GetColor(uint32_t defColor) const { return color == kInvalidColor ? defColor : color; }
  };

  StyleParams const * GetStyle(std::string styleUrl) const;

  StyleParams m_currStyle;
  std::map<std::string, StyleParams> m_styleParams;
  std::map<std::string, std::string> m_mapStyle2Style;

  int8_t m_attrCode;
  std::string m_attrId;
  std::string m_attrKey;

  LocalizableString m_name;
  LocalizableString m_description;

  PredefinedColor m_predefinedColor;
  BookmarkIcon m_icon;
  std::string m_iconPath;

  Timestamp m_timestamp;
  m2::PointD m_org;
  uint8_t m_viewportScale;
  std::vector<uint32_t> m_featureTypes;
  LocalizableString m_customName;
  std::vector<LocalId> m_boundTracks;
  LocalId m_localId;
  std::vector<TrackLayer> m_trackLayers;
  bool m_visible;
  std::string m_nearestToponym;
  std::vector<std::string> m_nearestToponyms;
  int m_minZoom = 1;
  kml::Properties m_properties;
  std::vector<CompilationId> m_compilations;
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
