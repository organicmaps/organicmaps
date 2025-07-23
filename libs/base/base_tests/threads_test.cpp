#include "testing/testing.hpp"

#include "base/thread.hpp"

#include <memory>
#include <vector>

using Vector = std::vector<int>;

static size_t summ = 0;
static size_t checkSumm = 0;
static size_t const MAX_COUNT = 1000000;

struct GeneratorThread : public threads::IRoutine
{
  explicit GeneratorThread(Vector & vec) : m_vec(vec) {}

  virtual void Do()
  {
    for (size_t i = 0; i < MAX_COUNT; ++i)
    {
      m_vec.push_back(static_cast<int>(i));
      summ += i;
    }
  }
  Vector & m_vec;
};

struct ReaderThread : public threads::IRoutine
{
  explicit ReaderThread(Vector & vec) : m_vec(vec) {}

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
  bool ok = reader.Create(std::make_unique<GeneratorThread>(vec));
  TEST(ok, ("Create Generator thread"));

  reader.Join();

  threads::Thread writer;
  ok = writer.Create(std::make_unique<ReaderThread>(vec));
  TEST(ok, ("Create Reader thread"));

  writer.Join();

  TEST_EQUAL(vec.size(), MAX_COUNT, ("vector size"));
  TEST_EQUAL(summ, checkSumm, ("check summ"));
}

class SomeClass
{
  DISALLOW_COPY(SomeClass);

public:
  SomeClass() {}
  void Increment(int * a, int b) { *a = *a + b; }
};

static void Increment(int * a, int b)
{
  *a = *a + b;
}

UNIT_TEST(SimpleThreadTest1)
{
  int a = 0;

  auto fn = [&a]() { a = 1; };

  threads::SimpleThread t(fn);
  t.join();

  TEST_EQUAL(a, 1, ("test a"));
}

UNIT_TEST(SimpleThreadTest2)
{
  int a = 0;

  threads::SimpleThread t([&a]() { a = 1; });
  t.join();

  TEST_EQUAL(a, 1, ("test a"));
}

UNIT_TEST(SimpleThreadTest3)
{
  int a = 0;

  SomeClass instance;
  threads::SimpleThread t(&SomeClass::Increment, &instance, &a, 1);
  t.join();

  TEST_EQUAL(a, 1, ("test a"));
}

UNIT_TEST(SimpleThreadTest4)
{
  int a = 0;

  threads::SimpleThread t(&Increment, &a, 1);
  t.join();

  TEST_EQUAL(a, 1, ("test a"));
}
