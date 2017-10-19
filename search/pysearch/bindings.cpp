#include "search/engine.hpp"
#include "search/processor_factory.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "indexer/classificator_loader.hpp"

#include "storage/country.hpp"
#include "storage/country_info_getter.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

#include <boost/python.hpp>

#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "defines.hpp"

using namespace std;

namespace
{
unique_ptr<storage::TMappingAffiliations> g_affiliations;

void Init(string const & resource_path, string const & mwm_path)
{
  auto & platform = GetPlatform();

  string countriesFile = COUNTRIES_FILE;

  if (!resource_path.empty())
  {
    platform.SetResourceDir(resource_path);
    countriesFile = my::JoinFoldersToPath(resource_path, COUNTRIES_FILE);
  }

  if (!mwm_path.empty())
    platform.SetWritableDirForTests(mwm_path);

  classificator::Load();

  g_affiliations = my::make_unique<storage::TMappingAffiliations>();
  storage::TCountryTree countries;
  auto const rv = storage::LoadCountriesFromFile(countriesFile, countries, *g_affiliations);
  CHECK(rv != -1, ("Can't load countries from:", countriesFile));
}

struct Mercator
{
  Mercator() = default;
  Mercator(double x, double y) : m_x(x), m_y(y) {}
  Mercator(m2::PointD const & m): m_x(m.x), m_y(m.y) {}

  string ToString() const
  {
    ostringstream os;
    os << "x: " << m_x << ", y: " << m_y;
    return os.str();
  }

  double m_x = 0.0;
  double m_y = 0.0;
};

struct Viewport
{
  Viewport() = default;
  Viewport(Mercator const & min, Mercator const & max) : m_min(min), m_max(max) {}

  string ToString() const
  {
    ostringstream os;
    os << "[" << m_min.ToString() << ", " << m_max.ToString() << "]";
    return os.str();
  }

  Mercator m_min;
  Mercator m_max;
};

struct Params
{
  string ToString() const
  {
    ostringstream os;
    os << m_query << ", " << m_locale << ", " << m_position.ToString() << ", "
       << m_viewport.ToString();
    return os.str();
  }

  string m_query;
  string m_locale;
  Mercator m_position;
  Viewport m_viewport;
};

struct Result
{
  Result() = default;

  Result(search::Result const & r)
  {
    m_name = r.GetString();
    m_address = r.GetAddress();
    m_hasCenter = r.HasPoint();
    if (m_hasCenter)
      m_center = r.GetFeatureCenter();
  }

  string ToString() const
  {
    ostringstream os;
    os << m_name << " [ " << m_address;
    if (m_hasCenter)
      os << ", " << m_center.ToString();
    os << " ]";
    return os.str();
  }

  string m_name;
  string m_address;
  bool m_hasCenter = false;
  Mercator m_center;
};

struct SearchEngineProxy
{
  SearchEngineProxy()
  {
    CHECK(g_affiliations.get(), ("init() was not called."));
    auto & platform = GetPlatform();
    auto infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(platform);
    infoGetter->InitAffiliationsInfo(&*g_affiliations);

    m_engine = make_shared<search::tests_support::TestSearchEngine>(
        move(infoGetter), my::make_unique<search::ProcessorFactory>(), search::Engine::Params{});

    vector<platform::LocalCountryFile> mwms;
    platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* the latest version */,
                                         mwms);
    for (auto & mwm : mwms)
    {
      mwm.SyncWithDisk();
      m_engine->RegisterMap(mwm);
    }
  }

  boost::python::list Query(Params const & params) const
  {
    m_engine->SetLocale(params.m_locale);

    search::SearchParams sp;
    sp.m_query = params.m_query;
    sp.m_inputLocale = params.m_locale;
    sp.m_mode = search::Mode::Everywhere;
    sp.SetPosition(MercatorBounds::YToLat(params.m_position.m_y),
                   MercatorBounds::XToLon(params.m_position.m_x));
    sp.m_suggestsEnabled = false;

    auto const & bottomLeft = params.m_viewport.m_min;
    auto const & topRight = params.m_viewport.m_max;
    m2::RectD const viewport(bottomLeft.m_x, bottomLeft.m_y, topRight.m_x, topRight.m_y);
    search::tests_support::TestSearchRequest request(*m_engine, sp, viewport);
    request.Run();

    boost::python::list results;
    for (auto const & result : request.Results())
      results.append(Result(result));
    return results;
  }

  shared_ptr<search::tests_support::TestSearchEngine> m_engine;
};
}  // namespace

BOOST_PYTHON_MODULE(pysearch)
{
  using namespace boost::python;

  def("init", &Init);

  class_<Mercator>("Mercator")
      .def(init<double, double>())
      .def_readwrite("x", &Mercator::m_x)
      .def_readwrite("y", &Mercator::m_y)
      .def("to_string", &Mercator::ToString);

  class_<Viewport>("Viewport")
      .def(init<Mercator const &, Mercator const &>())
      .def_readwrite("min", &Viewport::m_min)
      .def_readwrite("max", &Viewport::m_max)
      .def("to_string", &Viewport::ToString);

  class_<Params>("Params")
      .def_readwrite("query", &Params::m_query)
      .def_readwrite("locale", &Params::m_locale)
      .def_readwrite("position", &Params::m_position)
      .def_readwrite("viewport", &Params::m_viewport)
      .def("to_string", &Params::ToString);

  class_<Result>("Result")
      .def_readwrite("name", &Result::m_name)
      .def_readwrite("address", &Result::m_address)
      .def_readwrite("has_center", &Result::m_hasCenter)
      .def_readwrite("center", &Result::m_center)
      .def("to_string", &Result::ToString);

  class_<SearchEngineProxy>("SearchEngine").def("query", &SearchEngineProxy::Query);
}
