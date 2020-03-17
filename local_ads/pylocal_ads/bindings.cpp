#include "local_ads/campaign.hpp"
#include "local_ads/campaign_serialization.hpp"

// This header should be included due to a python compilation error.
// pyport.h overwrites defined macros and replaces it with its own.
// However, in the OS X c++ libraries, these are not macros but functions,
// hence the error. See https://bugs.python.org/issue10910
#include <locale>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include "pyhelpers/module_version.hpp"
#include "pyhelpers/vector_uint8.hpp"
#include "pyhelpers/vector_list_conversion.hpp"

#include <boost/python.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#pragma GCC diagnostic pop

using namespace local_ads;

namespace
{
std::vector<uint8_t> PySerialize(boost::python::list const & cs, Version const version)
{
  auto const campaigns = pyhelpers::PythonListToStdVector<Campaign>(cs);
  return Serialize(campaigns, version);
}

boost::python::list PyDeserialize(std::vector<uint8_t> const & blob)
{
  auto const campaigns = Deserialize(blob);
  return pyhelpers::StdVectorToPythonList(campaigns);
}
}  // namespace

BOOST_PYTHON_MODULE(pylocal_ads)
{
  using namespace boost::python;
  scope().attr("__version__") = PYBINDINGS_VERSION;

  // Register the to-python converters.
  to_python_converter<std::vector<uint8_t>, vector_uint8t_to_str>();
  vector_uint8t_from_python_str();

  class_<Campaign>("Campaign", init<uint32_t, uint16_t, uint8_t, uint8_t, uint8_t>())
      .def(init<uint32_t, uint16_t, uint8_t>())
      .def_readonly("m_featureId", &Campaign::m_featureId)
      .def_readonly("m_iconId", &Campaign::m_iconId)
      .def_readonly("m_daysBeforeExpired", &Campaign::m_daysBeforeExpired)
      .def_readonly("m_minZoomLevel", &Campaign::m_minZoomLevel)
      .def_readonly("m_priority", &Campaign::m_priority);

  class_<std::vector<Campaign>>("CampaignList")
      .def(vector_indexing_suite<std::vector<Campaign>>());

  enum_<Version>("Version")
      .value("UNKNOWN", Version::Unknown)
      .value("V1", Version::V1)
      .value("V2", Version::V2)
      .value("LATEST", Version::Latest)
      .export_values();

  def("serialize", PySerialize);
  def("deserialize", PyDeserialize);
}
