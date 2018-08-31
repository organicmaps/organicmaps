#include "search/engine.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"
#include "search/tracer.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"

#include "storage/country.hpp"
#include "storage/country_info_getter.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"

#include <boost/python.hpp>

#include <iomanip>
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

string ToString(search::Tracer::Parse::TokenType type)
{
  using TokenType = search::Tracer::Parse::TokenType;

  switch (type)
  {
  case TokenType::TOKEN_TYPE_POI: return "poi";
  case TokenType::TOKEN_TYPE_BUILDING: return "building";
  case TokenType::TOKEN_TYPE_STREET: return "street";
  case TokenType::TOKEN_TYPE_UNCLASSIFIED: return "unclassified";
  case TokenType::TOKEN_TYPE_VILLAGE: return "village";
  case TokenType::TOKEN_TYPE_CITY: return "city";
  case TokenType::TOKEN_TYPE_STATE: return "state";
  case TokenType::TOKEN_TYPE_COUNTRY: return "country";
  case TokenType::TOKEN_TYPE_POSTCODE: return "postcode";
  case TokenType::TOKEN_TYPE_COUNT: return "count";
  }
}

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

  g_affiliations = make_unique<storage::TMappingAffiliations>();
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

struct TraceResult
{
  string ToString() const
  {
    ostringstream os;
    os << "parse: [" << strings::JoinStrings(m_parse, ", ") << "], ";
    os << "is_category: " << boolalpha << m_isCategory;
    return os.str();
  }

  vector<string> m_parse;
  bool m_isCategory = false;
};

unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetter()
{
  CHECK(g_affiliations.get(), ("init() was not called."));
  auto & platform = GetPlatform();
  auto infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(platform);
  infoGetter->InitAffiliationsInfo(&*g_affiliations);
  return infoGetter;
}

struct Context
{
  Context() : m_engine(m_dataSource, CreateCountryInfoGetter(), search::Engine::Params{}) {}
  // todo(@pimenov) Choose right type for 'm_dataSource'.
  FrozenDataSource m_dataSource;
  search::tests_support::TestSearchEngine m_engine;
};

struct SearchEngineProxy
{
  SearchEngineProxy() : m_context(make_shared<Context>())
  {
    vector<platform::LocalCountryFile> mwms;
    platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* the latest version */,
                                         mwms);
    for (auto & mwm : mwms)
    {
      mwm.SyncWithDisk();
      m_context->m_dataSource.RegisterMap(mwm);
    }
  }

  search::SearchParams MakeSearchParams(Params const & params) const
  {
    search::SearchParams sp;
    sp.m_query = params.m_query;
    sp.m_inputLocale = params.m_locale;
    sp.m_mode = search::Mode::Everywhere;
    sp.m_position = m2::PointD(params.m_position.m_x, params.m_position.m_y);
    sp.m_needAddress = true;

    auto const & bottomLeft = params.m_viewport.m_min;
    auto const & topRight = params.m_viewport.m_max;
    sp.m_viewport = m2::RectD(bottomLeft.m_x, bottomLeft.m_y, topRight.m_x, topRight.m_y);

    return sp;
  }

  boost::python::list Query(Params const & params) const
  {
    m_context->m_engine.SetLocale(params.m_locale);
    search::tests_support::TestSearchRequest request(m_context->m_engine, MakeSearchParams(params));
    request.Run();

    boost::python::list results;
    for (auto const & result : request.Results())
      results.append(Result(result));
    return results;
  }

  boost::python::list Trace(Params const &params) const
  {
    m_context->m_engine.SetLocale(params.m_locale);

    auto sp = MakeSearchParams(params);
    auto tracer = make_shared<search::Tracer>();
    sp.m_tracer = tracer;

    search::tests_support::TestSearchRequest request(m_context->m_engine, sp);
    request.Run();

    boost::python::list trs;
    for (auto const & parse : tracer->GetUniqueParses())
    {
      TraceResult tr;
      for (size_t i = 0; i < parse.m_ranges.size(); ++i)
      {
        auto const & range = parse.m_ranges[i];
        if (!range.Empty())
          tr.m_parse.push_back(ToString(static_cast<search::Tracer::Parse::TokenType>(i)));
      }
      tr.m_isCategory = parse.m_category;
      trs.append(tr);
    }

    return trs;
  }

  shared_ptr<Context> m_context;
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
      .def("__repr__", &Result::ToString);

  class_<TraceResult>("TraceResult")
      .def_readwrite("parse", &TraceResult::m_parse)
      .def_readwrite("is_category", &TraceResult::m_isCategory)
      .def("__repr__", &TraceResult::ToString);

  class_<SearchEngineProxy>("SearchEngine")
      .def("query", &SearchEngineProxy::Query)
      .def("trace", &SearchEngineProxy::Trace);
}
