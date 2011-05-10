#include "../../testing/testing.hpp"

#include "../feature_bucketer.hpp"

#include "../../indexer/feature.hpp"
#include "../../indexer/mercator.hpp"
#include "../../indexer/cell_id.hpp"
#include "../../indexer/classificator_loader.hpp"

#include "../../platform/platform.hpp"

#include "../../indexer/indexer_tests/feature_routine.hpp"

#include "../../base/stl_add.hpp"

namespace
{
  class PushBackFeatureDebugStringOutput
  {
  public:
    typedef map<string, vector<string> > * InitDataType;

    PushBackFeatureDebugStringOutput(string const & name, InitDataType const & initData)
      : m_pContainer(&((*initData)[name]))
    {
    }

    void operator() (FeatureBuilder1 const & fb)
    {
      FeatureType f;
      FeatureBuilder2Feature(
        static_cast<FeatureBuilder2 &>(const_cast<FeatureBuilder1 &>(fb)), f);
      m_pContainer->push_back(f.DebugString(0));
    }

  private:
    vector<string> * m_pContainer;
  };

  typedef feature::CellFeatureBucketer<
      PushBackFeatureDebugStringOutput,
      feature::SimpleFeatureClipper,
      MercatorBounds,
      RectId
  > FeatureBucketer;
}

UNIT_TEST(FeatureBucketerSmokeTest)
{
  // classificator is needed because inside bucketer we're use it in WorldMapGenerator
  // @TODO clean up or remove cell bucketer and replace with world countries bucketer
  classificator::Read(GetPlatform().ReadPathForFile("drawing_rules.bin"),
                      GetPlatform().ReadPathForFile("classificator.txt"),
                      GetPlatform().ReadPathForFile("visibility.txt"));

  map<string, vector<string> > out, expectedOut;
  FeatureBucketer bucketer(1, &out);

  FeatureBuilder2 fb;
  fb.AddPoint(m2::PointD(10, 10));
  fb.AddPoint(m2::PointD(20, 20));
  fb.AddType(0);
  fb.SetLinear();
  bucketer(fb);

  FeatureType f;
  FeatureBuilder2Feature(fb, f);
  expectedOut["3"].push_back(f.DebugString(0));
  TEST_EQUAL(out, expectedOut, ());

  vector<string> bucketNames;
  bucketer.GetBucketNames(MakeBackInsertFunctor(bucketNames));
  TEST_EQUAL(bucketNames, vector<string>(1, "3"), ());
}
