#include "testing/testing.hpp"
#include "base/commands_queue.hpp"
#include "base/fence_manager.hpp"
#include "std/bind.hpp"

void add_int(core::CommandsQueue::Environment const & env,
             int fenceID,
             FenceManager & fenceManager,
             int & i,
             int a,
             int ms)
{
  threads::Sleep(ms);
  if (env.IsCancelled())
    return;
  i += a;
  LOG(LINFO, ("add_int result:", i));

  fenceManager.signalFence(fenceID);
}

void join_mul_int(core::CommandsQueue::Environment const & env,
                  int fenceID,
                  FenceManager & fenceManager,
                  int & i,
                  int b,
                  int ms)
{
  threads::Sleep(ms);
  fenceManager.joinFence(fenceID);
  i *= b;
  LOG(LINFO, ("join_mul_int result: ", i));
}

UNIT_TEST(FenceManager_SimpleFence)
{
  FenceManager fenceManager(3);
  core::CommandsQueue queue(2);

  int i = 3;

  queue.Start();

  int fenceID = fenceManager.insertFence();

  queue.AddCommand(bind(&add_int, _1, fenceID, ref(fenceManager), ref(i), 5, 500));
  queue.AddCommand(bind(&join_mul_int, _1, fenceID, ref(fenceManager), ref(i), 3, 1));

  queue.Join();
  queue.Cancel();

  TEST(i == 24, ());
}

UNIT_TEST(FenceManager_JoinAlreadySignalled)
{
  FenceManager fenceManager(3);
  core::CommandsQueue queue(2);

  int i = 3;

  queue.Start();

  int fenceID = fenceManager.insertFence();

  queue.AddCommand(bind(&add_int, _1, fenceID, ref(fenceManager), ref(i), 5, 1));
  queue.AddCommand(bind(&join_mul_int, _1, fenceID, ref(fenceManager), ref(i), 3, 500));

  queue.Join();
  queue.Cancel();

  TEST(i == 24, ());
}

UNIT_TEST(FenceManager_SignalAlreadySignalled)
{
  FenceManager fenceManager(3);
  core::CommandsQueue queue(2);

  int i = 3;

  queue.Start();

  int fenceID = fenceManager.insertFence();

  fenceManager.signalFence(fenceID);

  queue.AddCommand(bind(&add_int, _1, fenceID, ref(fenceManager), ref(i), 5, 1));
  queue.AddCommand(bind(&join_mul_int, _1, fenceID, ref(fenceManager), ref(i), 3, 500));

  queue.Join();
  queue.Cancel();

  TEST(i == 24, ())
}
