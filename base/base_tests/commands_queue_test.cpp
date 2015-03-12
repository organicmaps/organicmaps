#include "../../testing/testing.hpp"
#include "../commands_queue.hpp"
#include "../macros.hpp"
#include "../thread.hpp"
#include "../logging.hpp"

#include "../../std/atomic.hpp"
#include "../../std/bind.hpp"

void add_int(core::CommandsQueue::Environment const & env,
             atomic<int> & i,
             int a)
{
  threads::Sleep(500);
  if (env.IsCancelled())
    return;
  i += a;
  LOG(LINFO, ("add_int result:", i));
}

void join_mul_int(core::CommandsQueue::Environment const & env,
                  shared_ptr<core::CommandsQueue::Command> const & command,
                  atomic<int> & i,
                  int b)
{
  command->join();
  int value = i;
  while (!i.compare_exchange_weak(value, value * b));
  LOG(LINFO, ("join_mul_int result: ", i));
}

void mul_int(core::CommandsQueue::Environment const & env, atomic<int> & i, int b)
{
  if (env.IsCancelled())
    return;
  int value = i;
  while (!i.compare_exchange_weak(value, value * b));
  LOG(LINFO, ("mul_int result: ", i));
}

UNIT_TEST(CommandsQueue_SetupAndPerformSimpleTask)
{
  core::CommandsQueue queue(1);

  atomic<int> i(3);

  queue.Start();

  queue.AddCommand(bind(&add_int, _1, ref(i), 5));
  queue.AddCommand(bind(&mul_int, _1, ref(i), 3));

  queue.Join();

//  threads::Sleep(1000);

  queue.Cancel();

  TEST(i == 24, ());
}

UNIT_TEST(CommandsQueue_SetupAndPerformSimpleTaskWith2Executors)
{
  core::CommandsQueue queue(2);

  atomic<int> i(3);

  queue.Start();

  queue.AddCommand(bind(&add_int, _1, ref(i), 5));
  queue.AddCommand(bind(&mul_int, _1, ref(i), 3));

  queue.Join();
  //  threads::Sleep(1000);

  queue.Cancel();

  TEST(i == 14, ());
}

UNIT_TEST(CommandsQueue_TestEnvironmentCancellation)
{
  core::CommandsQueue queue(2);

  atomic<int> i(3);

  queue.Start();

  queue.AddCommand(bind(&add_int, _1, ref(i), 5));
  queue.AddCommand(bind(&mul_int, _1, ref(i), 3));

  threads::Sleep(200);  //< after this second command is executed, first will be cancelled

  queue.Cancel();

  TEST(i == 9, ());
}

UNIT_TEST(CommandsQueue_TestJoinCommand)
{
  core::CommandsQueue queue0(1);
  core::CommandsQueue queue1(1);

  atomic<int> i(3);

  queue0.Start();
  queue1.Start();

  shared_ptr<core::CommandsQueue::Command> cmd =
      queue0.AddCommand(bind(&add_int, _1, ref(i), 5), true);
  queue1.AddCommand(bind(&join_mul_int, _1, cmd, ref(i), 3), false);

  queue0.Join();
  queue1.Join();

  queue0.Cancel();
  queue1.Cancel();

  TEST(i == 24, ("core::CommandsQueue::Command::join doesn't work"));
}
