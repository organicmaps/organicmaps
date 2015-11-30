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
  TestRoutine(df::TFeaturesInfo & features, df::MemoryFeatureIndex &index)
    : m_features(features)
    , m_index(index)
  {}

  virtual void Do()
  {
    vector<FeatureID> result;
    df::MemoryFeatureIndex::Lock lock(m_index);
    m_index.ReadFeaturesRequest(m_features, result);
  }

private:
  df::TFeaturesInfo & m_features;
  df::MemoryFeatureIndex & m_index;
};

void GenerateFeatures(df::TFeaturesInfo & features, int taskIndex)
{
  int const kCount = 10000;
  for (int i = 0; i < kCount; ++i)
    features.push_back(df::FeatureInfo(FeatureID(MwmSet::MwmId(), taskIndex * kCount / 2 + i)));
}

} // namespace

UNIT_TEST(MemoryFeatureIndex_MT_Test)
{
  const int TASK_COUNT = 10;
  threads::Condition cond;
  int counter = 0;
  threads::ThreadPool pool(4, bind(&JoinFinishFinction, _1, ref(cond), ref(counter)));

  df::MemoryFeatureIndex index;
  df::TFeaturesInfo features[TASK_COUNT];

  for (int i = 0; i < TASK_COUNT; ++i)
  {
    GenerateFeatures(features[i], i);
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
