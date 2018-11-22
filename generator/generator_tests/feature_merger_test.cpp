#include "testing/testing.hpp"

#include "generator/feature_merger.hpp"

#include "indexer/classificator_loader.hpp"

#include "coding/point_coding.hpp"

namespace
{
  typedef m2::PointD P;

  class VectorEmitter : public FeatureEmitterIFace
  {
    vector<FeatureBuilder1> m_vec;
  public:
    virtual void operator() (FeatureBuilder1 const & fb)
    {
      m_vec.push_back(fb);
    }

    size_t GetSize() const { return m_vec.size(); }

    void Check(uint32_t type, size_t count) const
    {
      size_t test = 0;
      for (size_t i = 0; i < m_vec.size(); ++i)
        if (m_vec[i].HasType(type))
          ++test;

      TEST_EQUAL(test, count, ());
    }
  };
}

UNIT_TEST(FeatureMerger_MultipleTypes)
{
  classificator::Load();

  P arrPt[] = { P(0, 0), P(1, 1), P(2, 2), P(3, 3) };
  size_t const count = ARRAY_SIZE(arrPt)-1;

  FeatureBuilder1 arrF[count];

  for (size_t i = 0; i < count; ++i)
  {
    arrF[i].SetLinear();
    arrF[i].AddPoint(arrPt[i]);
    arrF[i].AddPoint(arrPt[i+1]);

    arrF[i].AddType(0);
  }

  arrF[0].AddType(1);
  arrF[1].AddType(1);
  arrF[0].AddType(2);
  arrF[1].AddType(2);

  arrF[1].AddType(3);
  arrF[2].AddType(3);
  arrF[1].AddType(4);
  arrF[2].AddType(4);

  FeatureMergeProcessor processor(kPointCoordBits);

  for (size_t i = 0; i < count; ++i)
    processor(arrF[i]);

  VectorEmitter emitter;
  processor.DoMerge(emitter);

  TEST_EQUAL(emitter.GetSize(), 3, ());

  emitter.Check(0, 1);
  emitter.Check(1, 1);
  emitter.Check(2, 1);
  emitter.Check(3, 1);
  emitter.Check(4, 1);
}

UNIT_TEST(FeatureMerger_Branches)
{
  classificator::Load();

  /* Try to unite next configuration
          o
         /\
     o--o--o--o
         \/
         o
  */

  vector<FeatureBuilder1> vF;

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(-2, 0));
  vF.back().AddPoint(P(-1, 0));

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(-1, 0));
  vF.back().AddPoint(P(0, 1));

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(-1, 0));
  vF.back().AddPoint(P(0, 0));

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(-1, 0));
  vF.back().AddPoint(P(0, -1));

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(0, 1));
  vF.back().AddPoint(P(1, 0));

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(0, 0));
  vF.back().AddPoint(P(1, 0));

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(0, -1));
  vF.back().AddPoint(P(1, 0));

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(1, 0));
  vF.back().AddPoint(P(2, 0));

  FeatureMergeProcessor processor(kPointCoordBits);

  for (size_t i = 0; i < vF.size(); ++i)
  {
    vF[i].SetLinear();
    vF[i].AddType(0);

    processor(vF[i]);
  }

  VectorEmitter emitter;
  processor.DoMerge(emitter);

  TEST_LESS_OR_EQUAL(emitter.GetSize(), 2, ());
}

UNIT_TEST(FeatureMerger_Rounds)
{
  classificator::Load();

  vector<FeatureBuilder1> vF;

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(-10, 0));
  vF.back().AddPoint(P(-5, 0));

  // make first round feature
  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(-4, 1));
  vF.back().AddPoint(P(-3, 0));
  vF.back().AddPoint(P(-4, -1));
  vF.back().AddPoint(P(-5, 0));
  vF.back().AddPoint(P(-4, 1));

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(-3, 0));
  vF.back().AddPoint(P(3, 0));

  // make second round feature
  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(4, -1));
  vF.back().AddPoint(P(3, 0));
  vF.back().AddPoint(P(4, 1));
  vF.back().AddPoint(P(5, 0));
  vF.back().AddPoint(P(4, -1));

  vF.push_back(FeatureBuilder1());
  vF.back().AddPoint(P(5, 0));
  vF.back().AddPoint(P(10, 0));

  FeatureMergeProcessor processor(kPointCoordBits);

  for (size_t i = 0; i < vF.size(); ++i)
  {
    vF[i].SetLinear();
    vF[i].AddType(0);

    processor(vF[i]);
  }

  VectorEmitter emitter;
  processor.DoMerge(emitter);

  TEST_EQUAL(emitter.GetSize(), 1, ());
}
