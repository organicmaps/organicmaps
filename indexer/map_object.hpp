#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftraits.hpp"

#include "geometry/latlon.hpp"

#include "coding/string_utf8_multilang.hpp"

#include <string>
#include <vector>

namespace osm
{
class EditableMapObject;

/// OSM internet_access tag values.
enum class Internet
{
  Unknown,  //!< Internet state is unknown (default).
  Wlan,     //!< Wireless Internet access is present.
  Terminal, //!< A computer with internet service.
  Wired,    //!< Wired Internet access is present.
  Yes,      //!< Unspecified Internet access is available.
  No        //!< There is definitely no any Internet access.
};
std::string DebugPrint(Internet internet);
/// @param[in]  inet  Should be lowercase like in DebugPrint.
Internet InternetFromString(std::string_view inet);

class MapObject
{
public:
  static char const * kFieldsSeparator;
  static constexpr uint8_t kMaxStarsCount = 7;

  void SetFromFeatureType(FeatureType & ft);

  FeatureID const & GetID() const;

  ms::LatLon GetLatLon() const;
  m2::PointD const & GetMercator() const;
  std::vector<m2::PointD> const & GetTriangesAsPoints() const;
  std::vector<m2::PointD> const & GetPoints() const;

  feature::TypesHolder const & GetTypes() const;
  std::string_view GetDefaultName() const;
  StringUtf8Multilang const & GetNameMultilang() const;

  std::string const & GetHouseNumber() const;
  std::string_view GetPostcode() const;

  /// @name Metadata fields.
  /// @todo Can implement polymorphic approach here and store map<MetadataID, MetadataEntryIFace>.
  /// All store/load/valid operations will be via MetadataEntryIFace interface instead of switch-case.
  /// @{
  using MetadataID = feature::Metadata::EType;

  std::string_view GetMetadata(MetadataID type) const;

  template <class FnT> void ForEachMetadataReadable(FnT && fn) const
  {
    m_metadata.ForEach([&fn](MetadataID id, std::string const & value)
    {
      switch (id)
      {
      case MetadataID::FMD_WIKIPEDIA: fn(id, feature::Metadata::ToWikiURL(value)); break;
      case MetadataID::FMD_WIKIMEDIA_COMMONS: fn(id, feature::Metadata::ToWikimediaCommonsURL(value)); break;
      /// @todo Skip description for now, because it's not a valid UTF8 string.
      /// @see EditableMapObject::ForEachMetadataItem.
      case MetadataID::FMD_DESCRIPTION: break;
      default: fn(id, value); break;
      }
    });
  }

  /// @returns non-localized cuisines keys.
  std::vector<std::string> GetCuisines() const;
  /// @returns translated cuisine(s).
  std::vector<std::string> GetLocalizedCuisines() const;
  /// @returns non-localized recycling type(s).
  std::vector<std::string> GetRecyclingTypes() const;
  /// @returns translated recycling type(s).
  std::vector<std::string> GetLocalizedRecyclingTypes() const;
  /// @returns translated and formatted cuisines.
  std::string FormatCuisines() const;

  std::string FormatRoadShields() const;

  std::string_view GetOpeningHours() const;
  Internet GetInternet() const;
  int GetStars() const;
  ftraits::WheelchairAvailability GetWheelchairType() const;

  /// @returns formatted elevation in feet or meters, or empty string.
  std::string GetElevationFormatted() const;
  /// @}

  bool IsPointType() const;
  feature::GeomType GetGeomType() const { return m_geomType; };
  int8_t GetLayer() const { return m_layer; };

  /// @returns true if object is of building type.
  bool IsBuilding() const;

  void AssignMetadata(feature::Metadata & dest) const { dest = m_metadata; }

protected:
  /// @returns "the best" type to display in UI.
  std::string GetLocalizedType() const;

  FeatureID m_featureID;
  m2::PointD m_mercator;

  std::vector<m2::PointD> m_points;
  std::vector<m2::PointD> m_triangles;

  StringUtf8Multilang m_name;
  std::string m_houseNumber;
  std::vector<std::string> m_roadShields;
  feature::TypesHolder m_types;
  feature::Metadata m_metadata;

  feature::GeomType m_geomType = feature::GeomType::Undefined;
  int8_t m_layer = feature::LAYER_EMPTY;
};

}  // namespace osm
