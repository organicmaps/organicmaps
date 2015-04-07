#include "base/runner.hpp"
#include "base/logging.hpp"
#include "base/exception.hpp"

void threads::IRunner::CallAndCatchAll(RunnerFuncT const & f)
{
  try
  {
    f();
  }
  catch (RootException & e)
  {
    LOG(LERROR, ("Exception caught by runner", e.Msg()));
  }
  catch (::std::exception & e)
  {
    LOG(LERROR, ("Std exception caught by runner", e.what()));
  }
  catch (...)
  {
    LOG(LERROR, ("Unknown exception caught by runner"));
  }
}
