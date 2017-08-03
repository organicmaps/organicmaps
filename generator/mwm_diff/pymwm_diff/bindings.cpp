#include "generator/mwm_diff/diff.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include <boost/python.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#pragma GCC diagnostic pop

using namespace std;

BOOST_PYTHON_MODULE(pymwm_diff)
{
  using namespace boost::python;

  def("make_diff", generator::mwm_diff::MakeDiff);
  def("apply_diff", generator::mwm_diff::ApplyDiff);
}
