#include "generator/feature_builder.hpp"
#include "generator/osm_source.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

#include "gflags/gflags.h"

DEFINE_string(osm, "", "Input .o5m file");
DEFINE_string(out, "", "Output file path");

namespace
{
class Emitter : public EmitterBase
{
public:
  Emitter(std::vector<FeatureBuilder1> & features)
    : m_features(features)
  {
    LOG_SHORT(LINFO, ("OSM data:", FLAGS_osm));
  }

  // EmitterBase overrides:
  void operator()(FeatureBuilder1 & fb) override
  {
    if (!ftypes::IsFoodChecker::Instance()(fb.GetParams().m_Types) || fb.GetParams().name.IsEmpty())
    {
      ++m_stats.m_unexpectedFeatures;
      return;
    }

    switch (fb.GetGeomType())
    {
    case feature::GEOM_POINT: ++m_stats.m_restaurantsPoi; break;
    case feature::GEOM_AREA: ++m_stats.m_restaurantsBuilding; break;
    default: ++m_stats.m_unexpectedFeatures;
    }
    m_features.emplace_back(fb);
  }

  void GetNames(std::vector<std::string> & names) const override
  {
    // We do not need to create any data file. See generator_tool.cpp and osm_source.cpp.
    names.clear();
  }

  bool Finish() override
  {
    LOG_SHORT(LINFO, ("Number of restaurants: POI:", m_stats.m_restaurantsPoi,
                      "BUILDING:", m_stats.m_restaurantsBuilding,
                      "TOTAL:", m_features.size(),
                      "INVALID:", m_stats.m_unexpectedFeatures));
    return true;
  }

private:
  std::vector<FeatureBuilder1> & m_features;

  struct Stats
  {
    // Number of features of any "food type".
    uint32_t m_restaurantsPoi = 0;
    uint32_t m_restaurantsBuilding = 0;
    uint32_t m_unexpectedFeatures = 0;
  };

  Stats m_stats;
};

feature::GenerateInfo GetGenerateInfo()
{
  feature::GenerateInfo info;
  info.m_osmFileName = FLAGS_osm;
  info.SetNodeStorageType("map");
  info.SetOsmFileType("o5m");

  info.m_intermediateDir = my::GetDirectory(FLAGS_out);

  // Set other info params here.

  return info;
}

void DumpRestaurants(std::vector<FeatureBuilder1> const & features, std::ostream & out)
{
  for (auto const & f : features)
  {
    auto const multilangName = f.GetParams().name;

    std::string defaultName;
    std::vector<std::string> translations;
    multilangName.ForEach(
        [&translations, &defaultName](uint8_t const langCode, std::string const & name) {
          if (langCode == StringUtf8Multilang::kDefaultCode)
          {
            defaultName = name;
            return;
          }
          translations.push_back(name);
        });
    auto const center = MercatorBounds::ToLatLon(f.GetKeyPoint());

    out << defaultName << '\t' << strings::JoinStrings(translations, '|') << '\t'
        << center.lat << ' ' << center.lon << '\t' << DebugPrint(f.GetGeomType()) << "\n";
  }
}
}  // namespace

int main(int argc, char * argv[])
{
  google::SetUsageMessage("Dump restaurants in tsv format.");

  if (argc == 1)
  {
    google::ShowUsageWithFlags(argv[0]);
    exit(0);
  }

  google::ParseCommandLineFlags(&argc, &argv, true);

  CHECK(!FLAGS_osm.empty(), ("Please specify osm path."));
  CHECK(!FLAGS_out.empty(), ("Please specify output file path."));

  classificator::Load();

  auto info = GetGenerateInfo();
  GenerateIntermediateData(info);

  std::vector<FeatureBuilder1> features;
  GenerateFeatures(info, [&features](feature::GenerateInfo const & /* info */)
  {
    return my::make_unique<Emitter>(features);
  });

  {
    std::ofstream ost(FLAGS_out);
    CHECK(ost.is_open(), ("Can't open file", FLAGS_out, strerror(errno)));
    DumpRestaurants(features, ost);
  }

  return 0;
}
