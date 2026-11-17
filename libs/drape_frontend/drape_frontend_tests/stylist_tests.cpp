#include "testing/testing.hpp"

#include "drape_frontend/relations_draw_info.hpp"
#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/map_object.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/shared_load_info.hpp"

#include "drape/hatching_decl.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/bit_streams.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/varint.hpp"

#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace
{
auto constexpr kStyles = []
{
  std::array<MapStyle, MapStyleCount - 1> styles{};
  for (size_t i = 0, s = 0; s < MapStyleCount; ++s)
    if (auto const style = static_cast<MapStyle>(s); style != MapStyleMerged)
      styles[i++] = style;
  return styles;
}();

class WaterArea : public osm::MapObject
{
public:
  explicit WaterArea(std::initializer_list<uint32_t> types)
  {
    m_geomType = feature::GeomType::Area;
    m_triangles = {{0.0, 0.0}, {0.01, 0.0}, {0.0, 0.01}};
    for (auto type : types)
      m_types.Add(type);
  }
};

class SampleFeature : public osm::MapObject
{
public:
  SampleFeature(uint32_t type, feature::GeomType geomType)
  {
    m_geomType = geomType;
    m_points = {{0.0, 0.0}, {0.01, 0.01}};
    m_triangles = {{0.0, 0.0}, {0.01, 0.0}, {0.0, 0.01}};
    m_types.Add(type);
    m_name.Add("en", "Sample");
    m_houseNumber = "12";
  }
};

class StyleSnapshotFixture
{
public:
  StyleSnapshotFixture() : m_originalStyle(GetStyleReader().GetCurrentStyle())
  {
    classificator::Load();
    for (auto const style : kStyles)
      classificator::EnsureStyleLoaded(style);
  }

  ~StyleSnapshotFixture() { GetStyleReader().SetCurrentStyle(m_originalStyle); }

private:
  MapStyle const m_originalStyle;
};

class SampleRelation : public feature::RouteRelationBase
{
public:
  explicit SampleRelation(Type type) { m_type = type; }
};

FeatureType MakeRelationMember(feature::SharedLoadInfo const & loader, uint32_t type,
                               std::vector<uint32_t> const & relations)
{
  FeatureParams params;
  params.SetType(type);
  params.SetGeomType(feature::GeomType::Line);
  std::vector<uint8_t> bytes;
  PushBackByteSink sink(bytes);
  params.Write(sink);
  {
    BitWriter header(sink);
    header.Write(2, 4);  // Two inline points.
    header.Write(0, 1);  // No external geometry.
    header.Write(!relations.empty(), 1);
  }
  serial::SaveInnerPath({{0.0, 0.0}, {0.01, 0.01}}, loader.GetDefGeometryCodingParams(), sink);
  if (!relations.empty())
    WriteVarUInt32SortedShortArray(relations, sink);
  return FeatureType(&loader, std::move(bytes));
}

void TestSameRules(df::Stylist const & actual, df::Stylist const & expected)
{
  TEST_EQUAL(actual.m_symbolRule, expected.m_symbolRule, ());
  TEST_EQUAL(actual.m_captionRule, expected.m_captionRule, ());
  TEST_EQUAL(actual.m_houseNumberRule, expected.m_houseNumberRule, ());
  TEST_EQUAL(actual.m_pathtextRule, expected.m_pathtextRule, ());
  TEST_EQUAL(actual.m_shieldRule, expected.m_shieldRule, ());
  TEST_EQUAL(actual.m_areaRule, expected.m_areaRule, ());
  TEST_EQUAL(actual.m_hatchingRule, expected.m_hatchingRule, ());
  TEST(std::equal(actual.m_lineRules.begin(), actual.m_lineRules.end(), expected.m_lineRules.begin(),
                  expected.m_lineRules.end()),
       ());
}

UNIT_TEST(Stylist_IsHatching)
{
  classificator::Load();
  auto const & cl = classif();

  auto const & checker = df::IsHatchingTerritoryChecker::Instance();

  TEST(checker(cl.GetTypeByPath({"boundary", "protected_area", "1"})), ());
  TEST(!checker(cl.GetTypeByPath({"boundary", "protected_area", "2"})), ());
  TEST(!checker(cl.GetTypeByPath({"boundary", "protected_area"})), ());

  TEST(checker(cl.GetTypeByPath({"boundary", "national_park"})), ());

  TEST(checker(cl.GetTypeByPath({"landuse", "military", "danger_area"})), ());

  TEST(checker(cl.GetTypeByPath({"amenity", "prison"})), ());
}

UNIT_TEST(Stylist_IsAreaPattern)
{
  classificator::Load();
  auto const & cl = classif();

  // Constructing the singleton resolves every checker type via GetTypeByPath, which CHECKs the type
  // exists - so an invalid path (e.g. the 3-level natural=beach=sand mistaken for 2-level) fails here.
  auto const & checker = df::IsAreaPatternChecker::Instance();

  // Stipple: sandy / desert surfaces and intermittent water. natural=sand is a beach subtype, matched via the parent.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach", "sand"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "desert"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "water", "intermittent"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "basin", "intermittent"})), dp::kStipplePattern, ());

  // Speckle: rocky surfaces.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "scree"})), dp::kSpecklePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "bare_rock"})), dp::kSpecklePattern, ());

  // Grid: planted landuse.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "orchard"})), dp::kGridPattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "vineyard"})), dp::kGridPattern, ());

  // Permanent water and unrelated area types get no pattern.
  TEST(checker.GetPattern(cl.GetTypeByPath({"natural", "water"})).empty(), ());
  TEST(checker.GetPattern(cl.GetTypeByPath({"landuse", "basin"})).empty(), ());
}

UNIT_TEST(Stylist_IntermittentWaterAreaPriority)
{
  auto & reader = GetStyleReader();
  for (size_t s = 0; s < MapStyleCount; ++s)
  {
    auto const style = static_cast<MapStyle>(s);
    if (style == MapStyleMerged)
      continue;
    reader.SetCurrentStyle(style);
    classificator::Load();
    auto const & cl = classif();
    auto const lake = cl.GetTypeByPath({"natural", "water", "lake"});
    auto const intermittent = cl.GetTypeByPath({"natural", "water", "intermittent"});
    auto const ditch = cl.GetTypeByPath({"natural", "water", "ditch"});
    auto const river = cl.GetTypeByPath({"waterway", "river"});
    auto const basin = cl.GetTypeByPath({"landuse", "basin"});
    auto const intermittentBasin = cl.GetTypeByPath({"landuse", "basin", "intermittent"});
    auto const areaPriority = [style](std::initializer_list<uint32_t> types, uint8_t zoom)
    {
      auto feature = FeatureType::CreateFromMapObject(WaterArea(types));
      df::Stylist stylist(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
      CHECK(stylist.m_areaRule != nullptr, ());
      return stylist.m_areaRule->priority;
    };

    auto const permanentDepthPriority = areaPriority({lake}, 14);
    auto const intermittentDepthPriority = areaPriority({lake, intermittent}, 14);
    auto const ditchDepthPriority = areaPriority({ditch, intermittent}, 14);
    drule::KeysT lineKeys;
    feature::GetDrawRule({river}, 14, feature::GeomType::Line, lineKeys);
    auto const line =
        std::find_if(lineKeys.begin(), lineKeys.end(), [](drule::Key const & k) { return k.m_type == drule::line; });
    CHECK(line != lineKeys.end(), ());
    // BG-top scales style priority by 0.1; gaps above 8 stay distinct even with a 16-bit depth buffer.
    TEST_GREATER(intermittentDepthPriority - line->m_priority, 8, (style));
    TEST_GREATER(permanentDepthPriority - intermittentDepthPriority, 8, (style));
    TEST_GREATER(ditchDepthPriority - permanentDepthPriority, 8, (style));
    TEST_EQUAL(areaPriority({ditch, intermittent}, 11), intermittentDepthPriority, (style));
    TEST_LESS(areaPriority({intermittentBasin}, 14), areaPriority({basin}, 14), (style));
  }
  reader.SetCurrentStyle(kDefaultMapStyle);
  classificator::Load();
}

UNIT_CLASS_TEST(StyleSnapshotFixture, Stylist_ExplicitStyleIgnoresGlobalSwitches)
{
  using feature::GeomType;
  struct Sample
  {
    uint32_t m_type;
    GeomType m_geomType;
  };
  auto const & cl = classif(MapStyleDefaultLight);
  std::array const samples = {Sample{cl.GetTypeByPath({"highway", "motorway"}), GeomType::Line},
                              Sample{cl.GetTypeByPath({"highway", "path"}), GeomType::Line},
                              Sample{cl.GetTypeByPath({"amenity", "cafe"}), GeomType::Point},
                              Sample{cl.GetTypeByPath({"building"}), GeomType::Area},
                              Sample{cl.GetTypeByPath({"natural", "water", "lake"}), GeomType::Area}};

  for (auto const style : kStyles)
  {
    for (auto const & sample : samples)
    {
      auto feature = FeatureType::CreateFromMapObject(SampleFeature(sample.m_type, sample.m_geomType));
      bool hasRules = false;
      for (uint8_t zoom = 1; zoom <= 19; ++zoom)
      {
        GetStyleReader().SetCurrentStyle(style);
        df::Stylist const expected(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
        hasRules = hasRules || expected.m_symbolRule || expected.m_areaRule || !expected.m_lineRules.empty();
        for (auto const current : kStyles)
        {
          GetStyleReader().SetCurrentStyle(current);
          df::Stylist const actual(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
          TestSameRules(actual, expected);
        }
      }
      TEST(hasRules, (style, sample.m_type));
    }
  }
}

UNIT_CLASS_TEST(StyleSnapshotFixture, CyclingStyle_OverviewNetwork)
{
  auto const & cl = classif(MapStyleDefaultLight);
  for (auto const style : {MapStyleCyclingLight, MapStyleCyclingDark})
  {
    for (auto const path : {"track", "path", "footway"})
    {
      auto feature =
          FeatureType::CreateFromMapObject(SampleFeature(cl.GetTypeByPath({"highway", path}), feature::GeomType::Line));
      for (uint8_t zoom = 11; zoom <= 13; ++zoom)
      {
        df::Stylist const ordinary(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
        TEST(ordinary.m_lineRules.empty(), (style, path, zoom));
      }
      auto const defaultStyle = GetMapStyleForMode(MapStyleMode::Default, MapStyleIsDark(style));
      for (uint8_t zoom = 14; zoom <= 19; ++zoom)
      {
        df::Stylist const ordinary(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
        df::Stylist const reference(*feature, zoom, StringUtf8Multilang::kEnglishCode, defaultStyle);
        TEST_EQUAL(ordinary.m_lineRules.size(), reference.m_lineRules.size(), (style, path, zoom));
        for (size_t i = 0; i < ordinary.m_lineRules.size(); ++i)
        {
          TEST_EQUAL(ordinary.m_lineRules[i]->width, reference.m_lineRules[i]->width, (style, path, zoom));
          TEST_EQUAL(ordinary.m_lineRules[i]->color, reference.m_lineRules[i]->color, (style, path, zoom));
        }
      }
    }

    auto const testCycleway = [&](base::StringIL path, uint8_t firstZoom)
    {
      auto feature = FeatureType::CreateFromMapObject(SampleFeature(cl.GetTypeByPath(path), feature::GeomType::Line));
      for (uint8_t zoom = firstZoom; zoom <= 19; ++zoom)
      {
        df::Stylist const cycling(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
        TEST(!cycling.m_lineRules.empty(), (style, path, zoom));
      }
    };
    testCycleway({"highway", "cycleway"}, 11);
    testCycleway({"highway", "path", "bicycle"}, 12);
    testCycleway({"highway", "footway", "bicycle"}, 13);
  }
}

UNIT_CLASS_TEST(StyleSnapshotFixture, CyclingStyle_RelationMembersAtOverviewZoom)
{
  using RouteType = feature::RouteRelationBase::Type;

  auto const path = GetPlatform().WritablePathForFile("cycling_style_relations.mwm");
  SCOPE_GUARD(removeFile, [&] { FileWriter::DeleteFileX(path); });
  feature::FeaturesOffsetsTable::Builder offsets;
  std::vector<uint8_t> relations;
  PushBackByteSink sink(relations);
  for (auto const type : {RouteType::Hiking, RouteType::Bicycle, RouteType::MTB})
  {
    offsets.PushOffset(static_cast<uint32_t>(relations.size()));
    SampleRelation(type).Write(sink);
  }
  {
    FilesContainerW writer(path);
    writer.Write(relations, RELATIONS_FILE_TAG);
  }

  auto const & hikingKey = df::RelationsDrawSettings::kHikingEnabledKey;
  std::string oldHikingSetting;
  bool const hadHikingSetting = settings::Get(hikingKey, oldHikingSetting);
  SCOPE_GUARD(restoreHiking, [&]
  {
    if (hadHikingSetting)
      settings::Set(hikingKey, oldHikingSetting);
    else
      settings::Delete(hikingKey);
  });
  settings::Set(hikingKey, true);

  FilesContainerR const container(path);
  feature::DataHeader const header;
  auto table = feature::FeaturesOffsetsTable::Build(offsets);
  feature::SharedLoadInfo loader(container, header, table.get(), nullptr /* metaDeserializer */);
  loader.m_version = feature::DatSectionHeader::Version::V2;
  auto const & cl = classif(MapStyleDefaultLight);
  auto const track = cl.GetTypeByPath({"highway", "track"});
  auto const cycleway = cl.GetTypeByPath({"highway", "cycleway"});
  auto bicycleMember = MakeRelationMember(loader, track, {1});
  auto mtbMember = MakeRelationMember(loader, track, {2});
  auto hikingMember = MakeRelationMember(loader, track, {0});
  auto plainTrack = MakeRelationMember(loader, track, {});
  auto hikingCycleway = MakeRelationMember(loader, cycleway, {0});
  auto sharedMember = MakeRelationMember(loader, cycleway, {0, 1});

  for (auto const style : {MapStyleCyclingLight, MapStyleCyclingDark})
  {
    for (uint8_t const zoom : {10, 11, 12})
    {
      auto const settings = df::RelationsDrawSettings::ForTile(style, zoom);
      auto const testMember = [&](FeatureType & member, bool expected)
      {
        auto const styled = df::MakeLineStylist(member, zoom, StringUtf8Multilang::kEnglishCode, style, settings);
        df::RelationsDrawInfo relationColors(settings);
        relationColors.Init(member);
        TEST_EQUAL(!styled.m_lineRules.empty(), expected, (style, zoom));
        TEST_EQUAL(relationColors.HasColors(), expected, (style, zoom));
      };
      testMember(bicycleMember, zoom >= 11);
      testMember(mtbMember, zoom >= 11);
      testMember(hikingMember, zoom >= 12);
      testMember(plainTrack, false);
    }

    uint8_t constexpr zoom = 12;
    auto const settings = df::RelationsDrawSettings::ForTile(style, zoom);
    for (auto * member : {&hikingCycleway, &sharedMember})
    {
      auto const styled = df::MakeLineStylist(*member, zoom, StringUtf8Multilang::kEnglishCode, style, settings);
      auto const baseStyle = df::Stylist(*member, zoom, StringUtf8Multilang::kEnglishCode, style);
      TEST(!baseStyle.m_lineRules.empty(), (style));
      TestSameRules(styled, baseStyle);
    }
  }
}

UNIT_CLASS_TEST(StyleSnapshotFixture, CyclingStyle_UsefulStopsAndCityDetail)
{
  auto const & cl = classif(MapStyleDefaultLight);
  for (auto const style : {MapStyleCyclingLight, MapStyleCyclingDark})
  {
    auto const testStop = [&](base::StringIL path, uint8_t zoom)
    {
      auto feature = FeatureType::CreateFromMapObject(SampleFeature(cl.GetTypeByPath(path), feature::GeomType::Point));
      df::Stylist const stop(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
      TEST(stop.m_symbolRule != nullptr, (style, path, zoom));
    };
    testStop({"amenity", "drinking_water"}, 12);
    testStop({"amenity", "fuel"}, 14);
    testStop({"amenity", "toilets"}, 15);
    testStop({"shop", "bicycle"}, 14);
    testStop({"amenity", "bicycle_repair_station"}, 14);
    testStop({"amenity", "bicycle_rental"}, 14);
    testStop({"amenity", "bicycle_parking"}, 15);
    testStop({"amenity", "compressed_air"}, 15);
    testStop({"shop", "supermarket"}, 16);

    for (auto const & path : {base::StringIL{"amenity", "parking"},
                              {"shop", "car"},
                              {"shop", "clothes"},
                              {"office", "lawyer"},
                              {"historic", "memorial"}})
    {
      auto feature = FeatureType::CreateFromMapObject(SampleFeature(cl.GetTypeByPath(path), feature::GeomType::Point));
      df::Stylist const distraction(*feature, 17, StringUtf8Multilang::kEnglishCode, style);
      TEST(distraction.m_symbolRule == nullptr && distraction.m_captionRule == nullptr, (style, path));
    }

    for (auto const & path : {base::StringIL{"amenity", "water_point", "drinking_water_no"},
                              {"man_made", "water_tap", "drinking_water_no"},
                              {"man_made", "water_well", "drinking_water_no"}})
    {
      auto feature = FeatureType::CreateFromMapObject(SampleFeature(cl.GetTypeByPath(path), feature::GeomType::Point));
      df::Stylist const water(*feature, 15, StringUtf8Multilang::kEnglishCode, style);
      TEST(water.m_symbolRule != nullptr && water.m_symbolRule->name == "drinking-water-no-m", (style, path));
    }

    for (auto const & path : {base::StringIL{"shop", "clothes"}, {"office", "lawyer"}})
    {
      auto feature = FeatureType::CreateFromMapObject(SampleFeature(cl.GetTypeByPath(path), feature::GeomType::Point));
      df::Stylist const closeUp(*feature, 18, StringUtf8Multilang::kEnglishCode, style);
      TEST(closeUp.m_symbolRule != nullptr, (style, path));
    }

    for (auto const & path : {base::StringIL{"tourism", "museum"}, {"historic", "castle"}})
    {
      auto feature = FeatureType::CreateFromMapObject(SampleFeature(cl.GetTypeByPath(path), feature::GeomType::Point));
      for (uint8_t zoom = 12; zoom <= 18; ++zoom)
      {
        df::Stylist const landmark(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
        TEST_EQUAL(landmark.m_symbolRule != nullptr, zoom == 18, (style, path, zoom));
      }
    }

    auto address = FeatureType::CreateFromMapObject(
        SampleFeature(cl.GetTypeByPath({"building", "address"}), feature::GeomType::Point));
    df::Stylist const destination(*address, 17, StringUtf8Multilang::kEnglishCode, style);
    TEST(destination.m_houseNumberRule != nullptr, (style));
  }
}
}  // namespace
