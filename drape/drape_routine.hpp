#pragma once

#include "base/macros.hpp"
#include "base/worker_thread.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

namespace dp
{
// This class MUST NOT run OpenGL-related tasks (which invoke OpenGL or contain any
// OpenGL data), use FR/BR threads for that.
class DrapeRoutine
{
  friend class Promise;

public:
  class Result
  {
  public:
    void Wait()
    {
      if (m_isFinished)
        return;

      DrapeRoutine::Instance().Wait(m_id);
    }

  private:
    friend class DrapeRoutine;
    explicit Result(uint64_t id) : m_id(id), m_isFinished(false) {}

    uint64_t Finish()
    {
      m_isFinished = true;
      return m_id;
    }

    uint64_t const m_id;
    std::atomic<bool> m_isFinished;
  };

  using ResultPtr = std::shared_ptr<Result>;

  static void Init()
  {
    Instance();
  }

  static void Shutdown()
  {
    Instance().FinishAll();
  }

  template <typename Task>
  static ResultPtr Run(Task && t)
  {
    ResultPtr result(new Result(Instance().GetNextId()));
    bool const success = Instance().m_workerThread.Push([result, t]() mutable
    {
      t();
      Instance().Notify(result->Finish());
    });

    if (!success)
      return {};

    return result;
  }

private:
  static DrapeRoutine & Instance()
  {
    static DrapeRoutine instance;
    return instance;
  }

  uint64_t GetNextId()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_counter++;
  }

  void Notify(uint64_t id)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_finishedId = id;
    m_condition.notify_all();
  }

  void Wait(uint64_t id)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_finished)
      return;
    m_condition.wait(lock, [this, id](){return m_finished || m_finishedId == id;});
  }

  void FinishAll()
  {
    m_workerThread.ShutdownAndJoin();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_finished = true;
    m_condition.notify_all();
  }

  uint64_t m_finishedId = 0;
  uint64_t m_counter = 0;
  bool m_finished = false;
  std::condition_variable m_condition;
  std::mutex m_mutex;
  base::WorkerThread m_workerThread;
};
}  // namespace dp
