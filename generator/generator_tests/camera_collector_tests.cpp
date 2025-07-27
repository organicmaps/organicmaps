#include "testing/testing.hpp"

#include "generator/collector_camera.hpp"
#include "generator/feature_maker.hpp"
#include "generator/filter_planet.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_source.hpp"
#include "generator/processor_factory.hpp"
#include "generator/raw_generator.hpp"
#include "generator/translator.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/map_style.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "base/macros.hpp"

#include "defines.hpp"

namespace routing_builder
{
using namespace generator;
using std::pair, std::string;

string const kSpeedCameraTag = R"(<tag k="highway" v="speed_camera"/>)";

feature::FeatureBuilder MakeFeatureBuilderWithParams(OsmElement & element)
{
  feature::FeatureBuilder fb;
  auto & params = fb.GetParams();
  ftype::GetNameAndType(&element, params);
  return fb;
}

class TranslatorForTest : public generator::Translator
{
public:
  explicit TranslatorForTest(std::shared_ptr<FeatureProcessorInterface> const & processor,
                             std::shared_ptr<generator::cache::IntermediateData> const & cache)
    : Translator(processor, cache, std::make_shared<FeatureMaker>(cache->GetCache()))
  {
    SetFilter(std::make_shared<FilterPlanet>());
  }

  // TranslatorInterface overrides:
  std::shared_ptr<TranslatorInterface> Clone() const override
  {
    CHECK(false, ());
    return {};
  }

  void Merge(TranslatorInterface const &) override { CHECK(false, ()); }

protected:
  using Translator::Translator;
};

class TestCameraCollector
{
public:
  // Directory name for creating test mwm and temporary files.
  string static const kTestDir;
  string static const kOsmFileName;

  TestCameraCollector()
  {
    GetStyleReader().SetCurrentStyle(MapStyleMerged);
    classificator::Load();
  }

  static bool Test(string const & osmSourceXML, std::set<pair<uint64_t, uint64_t>> const & trueAnswers)
  {
    using namespace platform::tests_support;

    Platform & platform = GetPlatform();
    WritableDirChanger writableDirChanger(kTestDir);
    auto const & writableDir = platform.WritableDir();
    ScopedDir const scopedDir(kTestDir);
    auto const osmRelativePath = base::JoinPath(kTestDir, kOsmFileName);
    ScopedFile const osmScopedFile(osmRelativePath, osmSourceXML);

    feature::GenerateInfo genInfo;
    // Generate intermediate data.
    genInfo.m_cacheDir = writableDir;
    genInfo.m_intermediateDir = writableDir;
    genInfo.m_nodeStorageType = feature::GenerateInfo::NodeStorageType::Index;
    genInfo.m_osmFileName = base::JoinPath(writableDir, osmRelativePath);
    genInfo.m_osmFileType = feature::GenerateInfo::OsmSourceType::XML;

    // Test save intermediate data is OK.
    CHECK(GenerateIntermediateData(genInfo), ());

    // Test load this data from cached file.
    generator::cache::IntermediateDataObjectsCache objectsCache;
    auto cache = std::make_shared<generator::cache::IntermediateData>(objectsCache, genInfo);
    auto collector =
        std::make_shared<CameraCollector>(genInfo.GetIntermediateFileName(CAMERAS_TO_WAYS_FILENAME), cache->GetCache());
    auto processor = CreateProcessor(ProcessorType::Noop);
    auto translator = std::make_shared<TranslatorForTest>(processor, cache);
    translator->SetCollector(collector);

    RawGenerator rawGenerator(genInfo);
    rawGenerator.GenerateCustom(translator);
    CHECK(rawGenerator.Execute(), ());
    std::set<pair<uint64_t, uint64_t>> answers;
    collector->ForEachCamera([&](auto const & camera)
    {
      for (auto const & w : camera.m_ways)
        answers.emplace(camera.m_id, w);
    });

    return answers == trueAnswers;
  }
};

string const TestCameraCollector::kTestDir = "camera_test";
string const TestCameraCollector::kOsmFileName = "planet" OSM_DATA_FILE_EXTENSION;

UNIT_CLASS_TEST(TestCameraCollector, test_1)
{
  string const osmSourceXML =
      R"(<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">
      <node id="1" lat="55.779384" lon="37.3699375" version="1">)" +
      kSpeedCameraTag + R"(</node><node id="2" lat="55.779304" lon="37.3699375" version="1">)" + kSpeedCameraTag +
      R"(</node><node id="3" lat="55.773084" lon="37.3699375" version="1">)" + kSpeedCameraTag +
      R"(</node><node id="4" lat="55.773084" lon="37.3699375" version="1">
      </node>
      <way id="10" version="1">
      <nd ref="1"/>
      <nd ref="4"/>
      <tag k="highway" v="unclassified"/>
      </way>
      <way id="20" version="1">
      <nd ref="1"/>
      <nd ref="2"/>
      <nd ref="3"/>
      <tag k="highway" v="unclassified"/>
      </way>
      </osm>)";

  std::set<pair<uint64_t, uint64_t>> trueAnswers = {{1, 10}, {1, 20}, {2, 20}, {3, 20}};

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}

UNIT_CLASS_TEST(TestCameraCollector, test_2)
{
  string const osmSourceXML =
      R"(<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">
      <node id="1" lat="55.779384" lon="37.3699375" version="1">)" +
      kSpeedCameraTag + R"(</node><node id="2" lat="55.779304" lon="37.3699375" version="1">)" + kSpeedCameraTag +
      R"(</node><node id="3" lat="55.773084" lon="37.3699375" version="1">)" + kSpeedCameraTag +
      R"(</node><node id="4" lat="55.773024" lon="37.3699375" version="1">)" + kSpeedCameraTag +
      R"(</node><node id="5" lat="55.773014" lon="37.3699375" version="1">)" + kSpeedCameraTag +
      R"(</node>
      <way id="10" version="1">
      <nd ref="1"/>
      <nd ref="2"/>
      <tag k="highway" v="unclassified"/>
      </way>
      <way id="20" version="1">
      <nd ref="1"/>
      <nd ref="3"/>
      <tag k="highway" v="unclassified"/>
      </way>
      <way id="30" version="1">
      <nd ref="1"/>
      <nd ref="3"/>
      <nd ref="4"/>
      <nd ref="5"/>
      <tag k="highway" v="unclassified"/>
      </way>
      </osm>)";

  std::set<pair<uint64_t, uint64_t>> trueAnswers = {{1, 10}, {2, 10}, {1, 20}, {3, 20},
                                                    {1, 30}, {3, 30}, {4, 30}, {5, 30}};

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}

UNIT_CLASS_TEST(TestCameraCollector, test_3)
{
  string const osmSourceXML =
      R"(<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">
      <node id="1" lat="55.779384" lon="37.3699375" version="1">)" +
      kSpeedCameraTag +
      R"(</node><node id="2" lat="55.779384" lon="37.3699375" version="1">
      </node>
      <way id="10" version="1">
      <nd ref="1"/>
      <nd ref="2"/>
      <tag k="highway" v="unclassified"/>
      </way>
      <way id="20" version="1">
      <nd ref="1"/>
      <nd ref="2"/>
      <tag k="highway" v="unclassified"/>
      </way>
      </osm>)";

  std::set<pair<uint64_t, uint64_t>> trueAnswers = {{1, 10}, {1, 20}};

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}

UNIT_CLASS_TEST(TestCameraCollector, test_4)
{
  string const osmSourceXML =
      R"(<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">
      <node id="1" lat="55.779384" lon="37.3699375" version="1">)" +
      kSpeedCameraTag +
      R"(</node><way id="10" version="1">
      <tag k="highway" v="unclassified"/>
      </way>
      <way id="20" version="1">
      <tag k="highway" v="unclassified"/>
      </way>
      </osm>)";

  std::set<pair<uint64_t, uint64_t>> trueAnswers = {};

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}

UNIT_CLASS_TEST(TestCameraCollector, test_5)
{
  string const osmSourceXML =
      R"(<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">
      <node id="1" lat="55.779384" lon="37.3699375" version="1"></node>
      <way id="10" version="1">
      <nd ref="1"/>
      <tag k="highway" v="unclassified"/>
      </way>
      </osm>)";

  std::set<pair<uint64_t, uint64_t>> trueAnswers = {};

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}

}  // namespace routing_builder
