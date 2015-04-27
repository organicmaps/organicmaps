#include "testing/testing.hpp"

#include "drape_frontend/memory_feature_index.hpp"

#include "base/thread.hpp"
#include "base/thread_pool.hpp"
#include "base/condition.hpp"

#include "std/vector.hpp"
#include "std/set.hpp"
#include "std/bind.hpp"

#include <cstdlib>
#include <ctime>

namespace
{
  void MarkNodesAsReaded(vector<df::FeatureInfo> & features, const vector<size_t> & indexes)
  {
    for (size_t i = 0; i < indexes.size(); ++i)
      features[indexes[i]].m_isOwner = true;
  }

//  void ResetReadedMark(vector<df::FeatureInfo> & features)
//  {
//    for (size_t i = 0; i < features.size(); ++i)
//      features[i].m_isOwner = false;
//  }
}

UNIT_TEST(MemoryFeatureIndex_SimpleTest)
{
//  df::TileInfo info1(1, 1, 10);
//  df::TileInfo info2(1, 2, 10);

//  set<FeatureID> features;

//  for (size_t i = 0; i < 10; ++i)
//  {
//    info1.m_featureInfo.push_back(df::FeatureInfo(FeatureID(0, i)));
//    info2.m_featureInfo.push_back(df::FeatureInfo(FeatureID(0, 2 * i)));

//    features.insert(info1.m_featureInfo.back().m_id);
//    features.insert(info2.m_featureInfo.back().m_id);
//  }

//  df::MemoryFeatureIndex index;

//  {
//    vector<size_t> result;
//    index.ReadFeaturesRequest(info1.m_featureInfo, result);
//    MarkNodesAsReaded(info1.m_featureInfo, result);
//  }

//  {
//    vector<size_t> result;
//    index.ReadFeaturesRequest(info2.m_featureInfo, result);
//    MarkNodesAsReaded(info2.m_featureInfo, result);
//  }

//  set<FeatureID> readedFeatures;
//  for (size_t i = 0; i < info1.m_featureInfo.size(); ++i)
//    TEST_EQUAL(readedFeatures.insert(info1.m_featureInfo[i].m_id).second, true, ());

//  for (size_t i = 0; i < info2.m_featureInfo.size(); ++i)
//  {
//    if (info2.m_featureInfo[i].m_isOwner)
//      TEST_EQUAL(readedFeatures.insert(info2.m_featureInfo[i].m_id).second, true, ());
//  }

//  TEST_EQUAL(readedFeatures.size(), features.size(), ());
}

UNIT_TEST(MemoryFeatureIndex_Test)
{
//  df::TileInfo info1(1, 1, 10);
//  df::TileInfo info2(1, 2, 10);

//  for (size_t i = 0; i < 10; ++i)
//  {
//    FeatureID id1(0, i);
//    FeatureID id2(0, 2 * i);

//    info1.m_featureInfo.push_back(df::FeatureInfo(FeatureID(0, i)));
//    info2.m_featureInfo.push_back(df::FeatureInfo(FeatureID(0, 2 * i)));
//  }

//  df::MemoryFeatureIndex index;

//  {
//    vector<size_t> result;
//    index.ReadFeaturesRequest(info1.m_featureInfo, result);
//    TEST_EQUAL(result.size(), info1.m_featureInfo.size(), ());
//    MarkNodesAsReaded(info1.m_featureInfo, result);
//  }

//  {
//    vector<size_t> result;
//    index.ReadFeaturesRequest(info2.m_featureInfo, result);
//    TEST_EQUAL(result.size(), 5, ());
//    MarkNodesAsReaded(info2.m_featureInfo, result);
//  }

//  index.RemoveFeatures(info1.m_featureInfo);
//  ResetReadedMark(info1.m_featureInfo);

//  {
//    vector<size_t> result;
//    index.ReadFeaturesRequest(info2.m_featureInfo, result);
//    TEST_EQUAL(result.size(), 5, ());
//    MarkNodesAsReaded(info2.m_featureInfo, result);
//  }

//  for (size_t i = 0; i < info2.m_featureInfo.size(); ++i)
//    TEST_EQUAL(info2.m_featureInfo[i].m_isOwner, true, ());

//  index.RemoveFeatures(info2.m_featureInfo);
//  ResetReadedMark(info2.m_featureInfo);

//  {
//    vector<size_t> result;
//    index.ReadFeaturesRequest(info1.m_featureInfo, result);
//    TEST_EQUAL(result.size(), info1.m_featureInfo.size(), ());
//    MarkNodesAsReaded(info1.m_featureInfo, result);
//  }

//  for (size_t i = 0; i < info1.m_featureInfo.size(); ++i)
//    TEST_EQUAL(info1.m_featureInfo[i].m_isOwner, true, ());
}

namespace
{
  void JoinFinishFinction(threads::IRoutine * routine, threads::Condition & condition, int & counter)
  {
    condition.Lock();
    counter++;
    condition.Unlock();
    delete routine;

    condition.Signal();
  }

  class TestRoutine : public threads::IRoutine
  {
  public:
    TestRoutine(vector<df::FeatureInfo> & features, df::MemoryFeatureIndex &index)
      : m_features(features)
      , m_index(index)
    {
    }

    virtual void Do()
    {
      vector<size_t> result;
      m_index.ReadFeaturesRequest(m_features, result);
      MarkNodesAsReaded(m_features, result);
    }

  private:
    vector<df::FeatureInfo> & m_features;
    df::MemoryFeatureIndex & m_index;
  };

  void GenerateFeatures(vector<df::FeatureInfo> & features)
  {
    for (int i = 0; i < 10000; ++i)
      features.push_back(df::FeatureInfo(FeatureID(MwmSet::MwmId(), rand())));
  }
}

UNIT_TEST(MemoryFeatureIndex_MT_Test)
{
  std::srand(std::time(0));
  const int TASK_COUNT = 10;
  threads::Condition cond;
  int counter = 0;
  threads::ThreadPool pool(4, bind(&JoinFinishFinction, _1, ref(cond), ref(counter)));

  df::MemoryFeatureIndex index;
  vector<df::FeatureInfo> features[TASK_COUNT];

  for (int i = 0; i < TASK_COUNT; ++i)
  {
    GenerateFeatures(features[i]);
    pool.PushBack(new TestRoutine(features[i], index));
  }

  while(true)
  {
    cond.Lock();
    if (counter == TASK_COUNT)
    {
      cond.Unlock();
      break;
    }

    cond.Wait();
    cond.Unlock();
  }

  set<FeatureID> allFeatures;
  set<FeatureID> readedFeatures;

  for (int i = 0; i < TASK_COUNT; ++i)
  {
    for (size_t j = 0; j < features[i].size(); ++j)
    {
      allFeatures.insert(features[i][j].m_id);
      if (features[i][j].m_isOwner == true)
        TEST_EQUAL(readedFeatures.insert(features[i][j].m_id).second, true, ());
    }
  }

  TEST_EQUAL(allFeatures.size(), readedFeatures.size(), ());
}
