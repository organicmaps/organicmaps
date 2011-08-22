#include "../../testing/testing.hpp"
#include "../commands_queue.hpp"
#include "../macros.hpp"
#include "../../std/bind.hpp"
#include "../thread.hpp"
#include "../logging.hpp"

void add_int(core::CommandsQueue::Environment const & env, int & i, int a)
{
  threads::Sleep(500);
  if (env.IsCancelled())
    return;
  i += a;
  LOG(LINFO, ("add_int result:", i));
}

void mul_int(core::CommandsQueue::Environment const & env, int & i, int b)
{
  if (env.IsCancelled())
    return;
  i *= b;
  LOG(LINFO, ("mul_int result:", i));
}

UNIT_TEST(CommandsQueue_SetupAndPerformSimpleTask)
{
  core::CommandsQueue queue(1);

  int i = 3;

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

  int i = 3;

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

  int i = 3;

  queue.Start();

  queue.AddCommand(bind(&add_int, _1, ref(i), 5));
  queue.AddCommand(bind(&mul_int, _1, ref(i), 3));

  threads::Sleep(200); //< after this second command is executed, first will be cancelled

  queue.Cancel();

  TEST(i == 9, ());
}
