#include "generator/mwm_diff/diff.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include "pyhelpers/module_version.hpp"

#include <boost/python.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#pragma GCC diagnostic pop

using namespace std;

namespace
{
// Applies the diff at |diffPath| to the mwm at |oldMwmPath|. The resulting
// mwm is stored at |newMwmPath|.
// It is assumed that the file at |oldMwmPath| is a valid mwm and the file
// at |diffPath| is a valid mwmdiff.
// Returns true on success and false on failure.
bool ApplyDiff(string const & oldMwmPath, string const & newMwmPath, string const & diffPath)
{
  base::Cancellable cancellable;
  auto const result = generator::mwm_diff::ApplyDiff(oldMwmPath, newMwmPath, diffPath, cancellable);

  switch (result)
  {
  case generator::mwm_diff::DiffApplicationResult::Ok:
    return true;
  case generator::mwm_diff::DiffApplicationResult::Failed:
    return false;
  case generator::mwm_diff::DiffApplicationResult::Cancelled:
    UNREACHABLE();
    break;
  }

  UNREACHABLE();
}
}  // namespace

BOOST_PYTHON_MODULE(pymwm_diff)
{
  using namespace boost::python;
  scope().attr("__version__") = PYBINDINGS_VERSION;

  def("make_diff", generator::mwm_diff::MakeDiff);
  def("apply_diff", ApplyDiff);
}
