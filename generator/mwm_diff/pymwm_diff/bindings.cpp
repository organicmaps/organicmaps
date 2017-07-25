#include "generator/mwm_diff/diff.hpp"

#include <boost/python.hpp>

using namespace std;

BOOST_PYTHON_MODULE(pymwm_diff)
{
  using namespace boost::python;

  def("make_diff", generator::mwm_diff::MakeDiff);
  def("apply_diff", generator::mwm_diff::ApplyDiff);
}
