#include "../../testing/testing.hpp"

#include "../worker_thread.hpp"

#include "../../std/memory.hpp"
#include "../../std/vector.hpp"

struct Task
{
  Task(vector<int> & buffer, int index) : m_buffer(buffer), m_index(index) {}

  void operator()() const { m_buffer.push_back(m_index); }

  vector<int> & m_buffer;
  int m_index;
};

UNIT_TEST(WorkerThread_Basic)
{
  my::WorkerThread<Task> thread(5 /* maxTasks */);

  vector<int> buffer;

  for (int i = 0; i < 10; ++i)
    thread.Push(make_shared<Task>(buffer, i));
  thread.RunUntilIdleAndStop();

  TEST_EQUAL(static_cast<size_t>(10), buffer.size(), ());
  for (size_t i = 0; i < buffer.size(); ++i)
    TEST_EQUAL(static_cast<int>(i), buffer[i], ());
}
