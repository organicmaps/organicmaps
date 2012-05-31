#include "../../testing/testing.hpp"

#include "../multiproducer_oneconsumer.hpp"


namespace
{
  class AccumulateTask : public MultiProducerOneConsumer::ITask
  {
    vector<uint64_t> & m_res;
  public:
    AccumulateTask(vector<uint64_t> & res) : m_res(res) {}

    static uint64_t const s_upper = 1000000;

    virtual void RunBase()
    {
      uint64_t summ = 0;
      for (uint64_t i = 0; i <= s_upper; ++i)
        summ += i;
      Emit(reinterpret_cast<void *>(summ));
    }

    virtual void EmitBase(void * p)
    {
      m_res.push_back(reinterpret_cast<uint64_t>(p));
    }
  };
}

UNIT_TEST(MultiProducers_Smoke)
{
  size_t const count = 64;
  vector<uint64_t> vec;

  MultiProducerOneConsumer runner(8);
  for (size_t i = 0; i < count; ++i)
    runner.RunTask(new AccumulateTask(vec));
  runner.Finish();

  TEST_EQUAL(vec.size(), count, ());
  uint64_t const res = AccumulateTask::s_upper * (AccumulateTask::s_upper + 1) / 2;
  for (size_t i = 0; i < count; ++i)
    TEST_EQUAL(vec[i], res, ());
}
