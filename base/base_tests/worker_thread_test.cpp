#include "../../testing/testing.hpp"

#include "../worker_thread.hpp"

#include "../../std/shared_ptr.hpp"
#include "../../std/vector.hpp"

struct Task
{
  Task(vector<size_t> & buffer, size_t index) : m_buffer(buffer), m_index(index) {}

  void operator()() const { m_buffer.push_back(m_index); }

  vector<size_t> & m_buffer;
  size_t m_index;
};

UNIT_TEST(WorkerThread_Basic)
{
  size_t const kNumTasks = 10;
  size_t const kMaxTasksInQueue = 5;

  my::WorkerThread<Task> thread(kMaxTasksInQueue);

  vector<size_t> buffer;
  for (size_t i = 0; i < kNumTasks; ++i)
    thread.Push(make_shared<Task>(buffer, i));
  thread.RunUntilIdleAndStop();

  TEST_EQUAL(kNumTasks, buffer.size(), ());
  for (size_t i = 0; i < buffer.size(); ++i)
    TEST_EQUAL(i, buffer[i], ());
}

UNIT_TEST(WorkerThread_NoStopCall)
{
  size_t const kNumTasks = 10;
  size_t const kMaxTasksInQueue = 5;
  vector<size_t> buffer;

  {
    my::WorkerThread<Task> thread(kMaxTasksInQueue);
    for (size_t i = 0; i < kNumTasks; ++i)
      thread.Push(make_shared<Task>(buffer, i));
  }

  TEST_EQUAL(kNumTasks, buffer.size(), ());
  for (size_t i = 0; i < buffer.size(); ++i)
    TEST_EQUAL(i, buffer[i], ());
}
