#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/map_object.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "std/vector.hpp"

namespace osm
{
/// Holds information to construct editor's UI.
struct EditableProperties
{
  EditableProperties() = default;
  EditableProperties(vector<feature::Metadata::EType> const & metadata,
                     bool name, bool address)
      : m_name(name),
        m_address(address),
        m_metadata(metadata)
  {
  }

  bool m_name = false;
  /// If true, enables editing of house number, street address and post code.
  bool m_address = false;
  vector<feature::Metadata::EType> m_metadata;
  bool IsEditable() const { return m_name || m_address || !m_metadata.empty(); }
};

struct LocalizedName
{
  LocalizedName(int8_t code, string const & name);
  LocalizedName(string const & langCode, string const & name);

  /// m_code, m_lang and m_langName are defined in StringUtf8Multilang.
  int8_t const m_code;
  /// Non-owning pointers to internal static char const * array.
  char const * const m_lang;
  char const * const m_langName;
  string const m_name;
};

/// Class which contains vector of localized names with following priority:
///  1. Names for Mwm languages
///  2. User`s language name
///  3. Other names
/// and mandatoryNamesCount - count of names which should be always shown.
struct NamesDataSource
{
  vector<LocalizedName> names;
  size_t mandatoryNamesCount = 0;
};

struct FakeName
{
  FakeName(int8_t code, string filledName)
    : m_code(code)
    , m_filledName(filledName)
  {
  }

  int8_t m_code;
  string m_filledName;
};
/// Contains information about fake names which were added for user convenience.
struct FakeNames
{
  void Clear()
  {
    m_names.clear();
    m_defaultName.clear();
  }

  vector<FakeName> m_names;
  string m_defaultName;
};

struct LocalizedStreet
{
  string m_defaultName;
  string m_localizedName;

  bool operator==(LocalizedStreet const & st) const { return m_defaultName == st.m_defaultName; }
};

class EditableMapObject : public MapObject
{
public:
  static int8_t const kMaximumLevelsEditableByUsers;

  bool IsNameEditable() const;
  bool IsAddressEditable() const;

  vector<Props> GetEditableProperties() const;
  // TODO(AlexZ): Remove this method and use GetEditableProperties() in UI.
  vector<feature::Metadata::EType> const & GetEditableFields() const;

  StringUtf8Multilang const & GetName() const;
  /// See comment for NamesDataSource class.
  NamesDataSource GetNamesDataSource(bool addFakes = true);
  LocalizedStreet const & GetStreet() const;
  vector<LocalizedStreet> const & GetNearbyStreets() const;
  string const & GetHouseNumber() const;
  string GetPostcode() const;
  string GetWikipedia() const;

  // These two methods should only be used in tests.
  uint64_t GetTestId() const;
  void SetTestId(uint64_t id);

  void SetEditableProperties(osm::EditableProperties const & props);
  //  void SetFeatureID(FeatureID const & fid);
  void SetName(StringUtf8Multilang const & name);
  void SetName(string name, int8_t langCode = StringUtf8Multilang::kDefaultCode);
  void SetMercator(m2::PointD const & center);
  void SetType(uint32_t featureType);
  void SetID(FeatureID const & fid);

  //  void SetTypes(feature::TypesHolder const & types);
  void SetStreet(LocalizedStreet const & st);
  void SetNearbyStreets(vector<LocalizedStreet> && streets);
  void SetHouseNumber(string const & houseNumber);
  void SetPostcode(string const & postcode);
  void SetPhone(string const & phone);
  void SetFax(string const & fax);

  void SetEmail(string const & email);
  void SetWebsite(string website);
  void SetWikipedia(string const & wikipedia);

  void SetInternet(Internet internet);
  void SetStars(int stars);
  void SetOperator(string const & op);

  void SetElevation(double ele);
  void SetFlats(string const & flats);

  void SetBuildingLevels(string const & buildingLevels);
  void SetLevel(string const & level);
  /// @param[in] cuisine is a vector of osm cuisine ids.
  void SetCuisines(vector<string> const & cuisine);
  void SetOpeningHours(string const & openingHours);

  /// Special mark that it's a point feature, not area or line.
  void SetPointType();
  /// Enables advanced mode with direct access to default name and disables any recalculations.
  void EnableNamesAdvancedMode() { m_namesAdvancedMode = true; }
  bool IsNamesAdvancedModeEnabled() const { return m_namesAdvancedMode; }
  /// Remove blank names and default name duplications.
  void RemoveBlankAndDuplicationsForDefault();
  /// Calls RemoveBlankNames or RemoveFakeNames depending on mode.
  void RemoveNeedlessNames();

  static bool ValidateBuildingLevels(string const & buildingLevels);
  static bool ValidateHouseNumber(string const & houseNumber);
  static bool ValidateFlats(string const & flats);
  static bool ValidatePostCode(string const & postCode);
  static bool ValidatePhoneList(string const & phone);
  static bool ValidateWebsite(string const & site);
  static bool ValidateEmail(string const & email);
  static bool ValidateLevel(string const & level);
  static bool ValidateName(string const & name);

  /// Check whether langCode can be used as default name.
  static bool CanUseAsDefaultName(int8_t const langCode, vector<int8_t> const & nativeMwmLanguages);

  /// See comment for NamesDataSource class.
  static NamesDataSource GetNamesDataSource(StringUtf8Multilang const & source,
                                            vector<int8_t> const & nativeMwmLanguages,
                                            int8_t const userLanguage);
  /// Removes fake names (which were added for user convenience) from name.
  static void RemoveFakeNames(FakeNames const & fakeNames, StringUtf8Multilang & name);

private:
  string m_houseNumber;
  LocalizedStreet m_street;
  vector<LocalizedStreet> m_nearbyStreets;
  EditableProperties m_editableProperties;
  FakeNames m_fakeNames;
  bool m_namesAdvancedMode = false;
};
}  // namespace osm
