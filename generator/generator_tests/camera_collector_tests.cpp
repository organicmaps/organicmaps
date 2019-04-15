#include "testing/testing.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "generator/collector_camera.hpp"
#include "generator/emitter_factory.hpp"
#include "generator/feature_maker.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/osm_source.hpp"
#include "generator/translator.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/map_style.hpp"

#include "base/macros.hpp"

#include "defines.hpp"

#include "std/string_view.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <utility>

using namespace generator;
using namespace generator::cache;
using namespace feature;
using namespace platform::tests_support;
using namespace platform;
using namespace routing;
using namespace std;

namespace
{
string const kSpeedCameraTag = "<tag k=\"highway\" v=\"speed_camera\"/>";

class TranslatorForTest : public Translator
{
public:
  explicit TranslatorForTest(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & cache,
                             feature::GenerateInfo const &)
    : Translator(emitter, cache, std::make_shared<FeatureMaker>(cache)) {}
};
}  // namespace

namespace generator_tests
{
class TestCameraCollector
{
public:
  // Directory name for creating test mwm and temprary files.
  std::string static const kTestDir;
  std::string static const kOsmFileName;

  TestCameraCollector()
  {
    GetStyleReader().SetCurrentStyle(MapStyleMerged);
    classificator::Load();
  }

  bool Test(string const & osmSourceXML, set<pair<uint64_t, uint64_t>> & trueAnswers)
  {
    Platform & platform = GetPlatform();
    WritableDirChanger writableDirChanger(kTestDir);
    auto const & writableDir = platform.WritableDir();
    ScopedDir const scopedDir(kTestDir);
    auto const osmRelativePath = base::JoinPath(kTestDir, kOsmFileName);
    ScopedFile const osmScopedFile(osmRelativePath, osmSourceXML);

    GenerateInfo genInfo;
    // Generate intermediate data.
    genInfo.m_intermediateDir = writableDir;
    genInfo.m_nodeStorageType = feature::GenerateInfo::NodeStorageType::Index;
    genInfo.m_osmFileName = base::JoinPath(writableDir, osmRelativePath);
    genInfo.m_osmFileType = feature::GenerateInfo::OsmSourceType::XML;

    // Test save intermediate data is OK.
    CHECK(GenerateIntermediateData(genInfo), ());

    // Test load this data from cached file.
    auto collector = std::make_shared<CameraCollector>(genInfo.GetIntermediateFileName(CAMERAS_TO_WAYS_FILENAME));
    CacheLoader cacheLoader(genInfo);
    auto emitter = CreateEmitter(EmitterType::Noop);
    TranslatorForTest translator(emitter, cacheLoader.GetCache(), genInfo);
    translator.AddCollector(collector);
    CHECK(GenerateRaw(genInfo, translator), ());

    set<pair<uint64_t, uint64_t>> answers;
    collector->m_processor.ForEachCamera([&](auto const & camera, auto const & ways) {
      for (auto const & w : ways)
        answers.emplace(camera.m_id, w);
    });

    return answers == trueAnswers;
  }
};

std::string const TestCameraCollector::kTestDir = "camera_test";
std::string const TestCameraCollector::kOsmFileName = "planet" OSM_DATA_FILE_EXTENSION;
} // namespace generator_tests

using namespace generator_tests;

UNIT_CLASS_TEST(TestCameraCollector, test_1)
{
  string const osmSourceXML = R"(
<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">

    <node id="1" lat="55.779384" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>
    <node id="2" lat="55.779304" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>
    <node id="3" lat="55.773084" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>
    <node id="4" lat="55.773084" lon="37.3699375" version="1"></node>

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

  </osm>
)";

  set<pair<uint64_t, uint64_t>> trueAnswers = {
    {1, 10}, {1, 20}, {2, 20}, {3, 20}
  };

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}

UNIT_CLASS_TEST(TestCameraCollector, test_2)
{
  string const osmSourceXML = R"(
<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">

    <node id="1" lat="55.779384" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>
    <node id="2" lat="55.779304" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>
    <node id="3" lat="55.773084" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>
    <node id="4" lat="55.773024" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>
    <node id="5" lat="55.773014" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>

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

  </osm>
)";

  set<pair<uint64_t, uint64_t>> trueAnswers = {
    {1, 10}, {2, 10}, {1, 20}, {3, 20}, {1, 30}, {3, 30}, {4, 30}, {5, 30}
  };

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}

UNIT_CLASS_TEST(TestCameraCollector, test_3)
{
  string const osmSourceXML = R"(
<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">

    <node id="1" lat="55.779384" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>
    <node id="2" lat="55.779384" lon="37.3699375" version="1"></node>

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

  </osm>
)";

  set<pair<uint64_t, uint64_t>> trueAnswers = {
    {1, 10}, {1, 20}
  };

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}

UNIT_CLASS_TEST(TestCameraCollector, test_4)
{
  string const osmSourceXML = R"(
<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">

    <node id="1" lat="55.779384" lon="37.3699375" version="1">)" + kSpeedCameraTag + R"(</node>

    <way id="10" version="1">
    <tag k="highway" v="unclassified"/>
    </way>
    <way id="20" version="1">
    <tag k="highway" v="unclassified"/>
    </way>

  </osm>
)";

  set<pair<uint64_t, uint64_t>> trueAnswers = {};

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}

UNIT_CLASS_TEST(TestCameraCollector, test_5)
{
  string const osmSourceXML = R"(
<osm version="0.6" generator="osmconvert 0.8.4" timestamp="2018-07-16T02:00:00Z">

    <node id="1" lat="55.779384" lon="37.3699375" version="1"></node>

    <way id="10" version="1">
      <nd ref="1"/>
      <tag k="highway" v="unclassified"/>
    </way>

  </osm>
)";

  set<pair<uint64_t, uint64_t>> trueAnswers = {};

  TEST(TestCameraCollector::Test(osmSourceXML, trueAnswers), ());
}
