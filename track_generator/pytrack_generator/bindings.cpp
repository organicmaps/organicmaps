#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_quality/utils.hpp"

#include "platform/platform.hpp"

#include "geometry/latlon.hpp"

#include <exception>
#include <sstream>
#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include "pyhelpers/module_version.hpp"
#include "pyhelpers/vector_list_conversion.hpp"

#include <boost/python.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

using namespace std;

namespace
{
class RouteNotFoundException : public exception
{
public:
  RouteNotFoundException(string const & msg) : m_msg(msg) {}

  virtual ~RouteNotFoundException() noexcept = default;

  char const * what() const noexcept override { return m_msg.c_str(); }

private:
  string m_msg;
};

PyObject * kRouteNotFoundException = nullptr;

PyObject * CreateExceptionClass(char const * name)
{
  using namespace boost::python;
  string const scopeName = extract<string>(scope().attr("__name__"));
  string const qualifiedName = scopeName + "." + name;
  PyObject * ex = PyErr_NewException(qualifiedName.c_str(), PyExc_Exception, nullptr);
  CHECK(ex, ());
  scope().attr(name) = handle<>(borrowed(ex));
  return ex;
}

template <typename Exception>
void Translate(PyObject * object, Exception const & e)
{
  PyErr_SetString(object, e.what());
}

struct Params
{
  Params(string const & data, string const & userResources) : m_dataPath(data), m_userResourcesPath(userResources)
  {
    if (m_dataPath.empty())
      throw runtime_error("data_path parameter not specified");

    if (m_userResourcesPath.empty())
      throw runtime_error("user_resources_path parameter not specified");
  }

  string DebugPrint() const
  {
    ostringstream ss;
    ss << "Params(data path: " << m_dataPath << ", user resources path: " << m_userResourcesPath << ")";
    return ss.str();
  }

  string m_dataPath;
  string m_userResourcesPath;
};

using Track = vector<ms::LatLon>;

Track GetTrackFrom(routing::Route const & route)
{
  CHECK(route.IsValid(), ());
  auto const & segments = route.GetRouteSegments();
  Track res;
  res.reserve(segments.size());
  for (auto const & s : segments)
    res.emplace_back(MercatorBounds::ToLatLon(s.GetJunction().GetPoint()));

  return res;
}

struct Generator
{
  explicit Generator(Params const & params) : m_params(params)
  {
    Platform & pl = GetPlatform();
    pl.SetWritableDirForTests(m_params.m_dataPath);
    pl.SetResourceDir(m_params.m_userResourcesPath);
  }

  Track Generate(boost::python::object const & iterable) const
  {
    using namespace routing_quality;

    Track const coordinates = python_list_to_std_vector<ms::LatLon>(iterable);
    auto result = GetRoute(FromLatLon(coordinates), routing::VehicleType::Pedestrian);
    if (result.m_code != routing::RouterResultCode::NoError)
      throw RouteNotFoundException("Can't build route");

    return GetTrackFrom(result.m_route);
  }

  Params m_params;
};
}  // namespace

using namespace boost::python;

BOOST_PYTHON_MODULE(pytrack_generator)
{
  scope().attr("__version__") = PYBINDINGS_VERSION;
  register_exception_translator<runtime_error>([](auto const & e) { Translate(PyExc_RuntimeError, e); });

  kRouteNotFoundException = CreateExceptionClass("RouteNotFoundException");
  register_exception_translator<RouteNotFoundException>([](auto const & e) { Translate(kRouteNotFoundException, e); });

  class_<Params>("Params", init<string, string>())
      .def("__str__", &Params::DebugPrint)
      .def_readonly("data_path", &Params::m_dataPath)
      .def_readonly("user_resources_path", &Params::m_userResourcesPath);

  class_<ms::LatLon>("LatLon", init<double, double>())
      .def("__str__", &ms::DebugPrint)
      .def_readonly("lat", &ms::LatLon::lat)
      .def_readonly("lon", &ms::LatLon::lon);

  class_<vector<ms::LatLon>>("LatLonList")
      .def(vector_indexing_suite<vector<ms::LatLon>>());

  class_<Generator>("Generator", init<Params>())
      .def("generate", &Generator::Generate)
      .def_readonly("params", &Generator::m_params);
}
