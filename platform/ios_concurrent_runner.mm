#include "concurrent_runner.hpp"

#include "../base/assert.hpp"
#include "../base/macros.hpp"
#include "../base/logging.hpp"

#include "../std/scoped_ptr.hpp"

#include <dispatch/dispatch.h>

namespace threads
{
  class ConcurrentRunner::Impl
  {
  public:
    dispatch_group_t m_group;

    Impl()
    {
      m_group = dispatch_group_create();
    }
    ~Impl()
    {
      dispatch_release(m_group);
    }

  };

  ConcurrentRunner::ConcurrentRunner() : m_pImpl(new ConcurrentRunner::Impl)
  {
  }

  ConcurrentRunner::~ConcurrentRunner()
  {
    delete m_pImpl;
  }

  // work-around for runtime boost exception, see
  // http://stackoverflow.com/questions/5438613/why-cant-i-use-a-boostfunction-in-an-objective-c-block
  struct BoostExceptionFixer
  {
    RunnerFuncT m_f;
    BoostExceptionFixer(RunnerFuncT const & f) : m_f(f) {}

    void operator()() const
    {
      scoped_ptr<BoostExceptionFixer> scopedThis;
      UNUSED_VALUE(scopedThis);

      IRunner<RunnerFuncT>::CallAndCatchAll(f);
    }
  };

  void ConcurrentRunner::Run(RunnerFuncT const & f) const
  {
    dispatch_queue_t globalQ = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    BoostExceptionFixer * tmp = new BoostExceptionFixer(f);
    dispatch_group_async(m_pImpl->m_group, globalQ, ^{
                         (*tmp)();
    });
  }

  void ConcurrentRunner::Join()
  {
    dispatch_group_wait(m_pImpl->m_group, DISPATCH_TIME_FOREVER);
  }
}
