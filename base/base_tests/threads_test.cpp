#include "../../testing/testing.hpp"

#include "../thread.hpp"
#include "../threads_pool.hpp"

#include "../../std/vector.hpp"

#include "../../base/start_mem_debug.hpp"

typedef std::vector<int> Vector;

static size_t summ = 0;
static size_t checkSumm = 0;
static size_t const MAX_COUNT = 1000000;

struct GeneratorThread : public threads::IRoutine
{
  GeneratorThread(Vector & vec) : m_vec(vec) {}

  virtual void Do()
  {
    for (size_t i = 0; i < MAX_COUNT; ++i)
    {
      m_vec.push_back(i);
      summ += i;
    }
  }
  Vector & m_vec;
};

struct ReaderThread : public threads::IRoutine
{
  ReaderThread(Vector & vec) : m_vec(vec) {}

  virtual void Do()
  {
    for (size_t i = 0; i < m_vec.size(); ++i)
      checkSumm += m_vec[i];
  }
  Vector & m_vec;
};


UNIT_TEST(Simple_Threads)
{
  Vector vec;

  threads::Thread reader;
  bool ok = reader.Create(new GeneratorThread(vec));
  TEST( ok, ("Create Generator thread") );

  reader.Join();

  threads::Thread writer;
  ok = writer.Create(new ReaderThread(vec));
  TEST( ok, ("Create Reader thread") );

  writer.Join();

  TEST_EQUAL(vec.size(), MAX_COUNT, ("vector size"));
  TEST_EQUAL(summ, checkSumm, ("check summ"));
}


struct PoolThread : public threads::IRoutine
{
  PoolThread(int * counter) : m_counter(counter) {}

  virtual void Do()
  {
    ++(*m_counter);
  }
  int * m_counter;
};


static int gPoolCounter = 0;

UNIT_TEST(Threads_Pool)
{
  threads::Pool pool;
  pool.StartThread(new PoolThread(&gPoolCounter));
  pool.StartThread(new PoolThread(&gPoolCounter));
  pool.StartThread(new PoolThread(&gPoolCounter));
  pool.StartThread(new PoolThread(&gPoolCounter));
  pool.StartThread(new PoolThread(&gPoolCounter));

  pool.Join();

  TEST_EQUAL(gPoolCounter, 5, ("All pool threads should finish"));
}
