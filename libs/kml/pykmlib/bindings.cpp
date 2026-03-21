#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/assert.hpp"

// This header should be included due to a python compilation error.
// pyport.h overwrites defined macros and replaces it with its own.
// However, in the OS X c++ libraries, these are not macros but functions,
// hence the error. See https://bugs.python.org/issue10910
#include <exception>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include "pyhelpers/module_version.hpp"
#include "pyhelpers/vector_list_conversion.hpp"
#include "pyhelpers/vector_uint8.hpp"

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#pragma GCC diagnostic pop

namespace
{
struct LocalizableStringAdapter
{
  static std::string const & Get(kml::LocalizableString const & str, std::string const & lang)
  {
    auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
    auto const it = str.find(langIndex);
    if (it != str.end())
      return it->second;
    throw std::runtime_error("Language not found. lang: " + lang);
  }

  static void Set(kml::LocalizableString & str, std::string const & lang, std::string const & val)
  {
    auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
    if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
      throw std::runtime_error("Unsupported language. lang: " + lang);
    str[langIndex] = val;
  }

  static void Delete(kml::LocalizableString & str, std::string const & lang)
  {
    auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
    auto const it = str.find(langIndex);
    if (it != str.end())
      str.erase(it);
    else
      throw std::runtime_error("Language not found. lang: " + lang);
  }

  static boost::python::dict GetDict(kml::LocalizableString const & str)
  {
    boost::python::dict d;
    for (auto const & s : str)
    {
      std::string lang = StringUtf8Multilang::GetLangByCode(s.first);
      if (lang.empty())
        throw std::runtime_error("Language not found");
      d[lang] = s.second;
    }
    return d;
  }

  static void SetDict(kml::LocalizableString & str, boost::python::dict & dict)
  {
    str.clear();
    if (dict.is_none())
      return;

    auto const langs = pyhelpers::PythonListToStdVector<std::string>(dict.keys());
    for (auto const & lang : langs)
    {
      auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
      if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
        throw std::runtime_error("Unsupported language. lang: " + lang);

      str[langIndex] = boost::python::extract<std::string>(dict[lang]);
    }
  }

  static std::string ToString(kml::LocalizableString const & str)
  {
    std::ostringstream out;
    out << "[";
    for (auto it = str.begin(); it != str.end(); ++it)
    {
      out << "'" << StringUtf8Multilang::GetLangByCode(it->first) << "':'" << it->second << "'";
      auto it2 = it;
      it2++;
      if (it2 != str.end())
        out << ", ";
    }
    out << "]";
    return out.str();
  }
};

std::string LatLonToString(ms::LatLon const & latLon);

struct PointWithAltitudeAdapter
{
  static m2::PointD const & GetPoint(geometry::PointWithAltitude const & ptWithAlt) { return ptWithAlt.GetPoint(); }

  static geometry::Altitude GetAltitude(geometry::PointWithAltitude const & ptWithAlt)
  {
    return ptWithAlt.GetAltitude();
  }

  static void SetPoint(geometry::PointWithAltitude & ptWithAlt, m2::PointD const & pt) { ptWithAlt.SetPoint(pt); }

  static void SetAltitude(geometry::PointWithAltitude & ptWithAlt, geometry::Altitude altitude)
  {
    ptWithAlt.SetAltitude(altitude);
  }

  static std::string ToString(geometry::PointWithAltitude const & ptWithAlt)
  {
    auto const latLon = mercator::ToLatLon(ptWithAlt.GetPoint());
    std::ostringstream out;
    out << "["
        << "point:" << LatLonToString(latLon) << ", "
        << "altitude:" << ptWithAlt.GetAltitude() << "]";
    return out.str();
  }
};

struct PropertiesAdapter
{
  static std::string const & Get(kml::Properties const & props, std::string const & key)
  {
    auto const it = props.find(key);
    if (it != props.end())
      return it->second;
    throw std::runtime_error("Property not found. key: " + key);
  }

  static void Set(kml::Properties & props, std::string const & key, std::string const & val) { props[key] = val; }

  static void Delete(kml::Properties & props, std::string const & key)
  {
    auto const it = props.find(key);
    if (it != props.end())
      props.erase(it);
    else
      throw std::runtime_error("Property not found. key: " + key);
  }

  static boost::python::dict GetDict(kml::Properties const & props)
  {
    boost::python::dict d;
    for (auto const & p : props)
      d[p.first] = p.second;
    return d;
  }

  static void SetDict(kml::Properties & props, boost::python::dict & dict)
  {
    props.clear();
    if (dict.is_none())
      return;

    auto const keys = pyhelpers::PythonListToStdVector<std::string>(dict.keys());
    for (auto const & k : keys)
      props[k] = boost::python::extract<std::string>(dict[k]);
  }

  static std::string ToString(kml::Properties const & props)
  {
    std::ostringstream out;
    out << "[";
    for (auto it = props.begin(); it != props.end(); ++it)
    {
      out << "'" << it->first << "':'" << it->second << "'";
      auto it2 = it;
      it2++;
      if (it2 != props.end())
        out << ", ";
    }
    out << "]";
    return out.str();
  }
};

template <typename T>
struct VectorAdapter
{
  static boost::python::list Get(std::vector<T> const & v) { return pyhelpers::StdVectorToPythonList(v); }

  static void Set(std::vector<T> & v, boost::python::object const & iterable)
  {
    if (iterable.is_none())
    {
      v.clear();
      return;
    }
    v = pyhelpers::PythonListToStdVector<T>(iterable);
  }

  static void PrintType(std::ostringstream & out, T const & t) { out << t; }

  static std::string ToString(std::vector<T> const & v)
  {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < v.size(); ++i)
    {
      PrintType(out, v[i]);
      if (i + 1 != v.size())
        out << ", ";
    }
    out << "]";
    return out.str();
  }
};

template <>
void VectorAdapter<uint8_t>::PrintType(std::ostringstream & out, uint8_t const & t)
{
  out << static_cast<uint32_t>(t);
}

template <>
void VectorAdapter<std::string>::PrintType(std::ostringstream & out, std::string const & s)
{
  out << "'" << s << "'";
}

std::string TrackLayerToString(kml::TrackLayer const & trackLayer);
template <>
void VectorAdapter<kml::TrackLayer>::PrintType(std::ostringstream & out, kml::TrackLayer const & t)
{
  out << TrackLayerToString(t);
}

template <>
void VectorAdapter<geometry::PointWithAltitude>::PrintType(std::ostringstream & out,
                                                           geometry::PointWithAltitude const & pt)
{
  out << PointWithAltitudeAdapter::ToString(pt);
}

std::string BookmarkDataToString(kml::BookmarkData const & bm);
template <>
void VectorAdapter<kml::BookmarkData>::PrintType(std::ostringstream & out, kml::BookmarkData const & bm)
{
  out << BookmarkDataToString(bm);
}

std::string TrackDataToString(kml::TrackData const & t);
template <>
void VectorAdapter<kml::TrackData>::PrintType(std::ostringstream & out, kml::TrackData const & t)
{
  out << TrackDataToString(t);
}

std::string CategoryDataToString(kml::CategoryData const & c);
template <>
void VectorAdapter<kml::CategoryData>::PrintType(std::ostringstream & out, kml::CategoryData const & c)
{
  out << CategoryDataToString(c);
}

std::string PredefinedColorToString(kml::PredefinedColor c)
{
  switch (c)
  {
  case kml::PredefinedColor::None: return "NONE";
  case kml::PredefinedColor::Red: return "RED";
  case kml::PredefinedColor::Blue: return "BLUE";
  case kml::PredefinedColor::Purple: return "PURPLE";
  case kml::PredefinedColor::Yellow: return "YELLOW";
  case kml::PredefinedColor::Pink: return "PINK";
  case kml::PredefinedColor::Brown: return "BROWN";
  case kml::PredefinedColor::Green: return "GREEN";
  case kml::PredefinedColor::Orange: return "ORANGE";
  case kml::PredefinedColor::DeepPurple: return "DEEPPURPLE";
  case kml::PredefinedColor::LightBlue: return "LIGHTBLUE";
  case kml::PredefinedColor::Cyan: return "CYAN";
  case kml::PredefinedColor::Teal: return "TEAL";
  case kml::PredefinedColor::Lime: return "LIME";
  case kml::PredefinedColor::DeepOrange: return "DEEPORANGE";
  case kml::PredefinedColor::Gray: return "GRAY";
  case kml::PredefinedColor::BlueGray: return "BLUEGRAY";
  case kml::PredefinedColor::Count: CHECK(false, ("Unknown predefined color")); return {};
  }
}

std::string AccessRulesToString(kml::AccessRules accessRules)
{
  switch (accessRules)
  {
  case kml::AccessRules::Local: return "LOCAL";
  case kml::AccessRules::DirectLink: return "DIRECT_LINK";
  case kml::AccessRules::P2P: return "P2P";
  case kml::AccessRules::Paid: return "PAID";
  case kml::AccessRules::Public: return "PUBLIC";
  case kml::AccessRules::AuthorOnly: return "AUTHOR_ONLY";
  case kml::AccessRules::Count: CHECK(false, ("Unknown access rules")); return {};
  }
}

std::string BookmarkIconToString(kml::BookmarkIcon icon)
{
  switch (icon)
  {
  case kml::BookmarkIcon::None: return "NONE";
  case kml::BookmarkIcon::Hotel: return "HOTEL";
  case kml::BookmarkIcon::Animals: return "ANIMALS";
  case kml::BookmarkIcon::Buddhism: return "BUDDHISM";
  case kml::BookmarkIcon::Building: return "BUILDING";
  case kml::BookmarkIcon::Christianity: return "CHRISTIANITY";
  case kml::BookmarkIcon::Entertainment: return "ENTERTAINMENT";
  case kml::BookmarkIcon::Exchange: return "EXCHANGE";
  case kml::BookmarkIcon::Food: return "FOOD";
  case kml::BookmarkIcon::Gas: return "GAS";
  case kml::BookmarkIcon::Judaism: return "JUDAISM";
  case kml::BookmarkIcon::Medicine: return "MEDICINE";
  case kml::BookmarkIcon::Mountain: return "MOUNTAIN";
  case kml::BookmarkIcon::Museum: return "MUSEUM";
  case kml::BookmarkIcon::Islam: return "ISLAM";
  case kml::BookmarkIcon::Park: return "PARK";
  case kml::BookmarkIcon::Parking: return "PARKING";
  case kml::BookmarkIcon::Shop: return "SHOP";
  case kml::BookmarkIcon::Sights: return "SIGHTS";
  case kml::BookmarkIcon::Swim: return "SWIM";
  case kml::BookmarkIcon::Water: return "WATER";
  case kml::BookmarkIcon::Bar: return "BAR";
  case kml::BookmarkIcon::Transport: return "TRANSPORT";
  case kml::BookmarkIcon::Viewpoint: return "VIEWPOINT";
  case kml::BookmarkIcon::Sport: return "SPORT";
  case kml::BookmarkIcon::Start: return "START";
  case kml::BookmarkIcon::Finish: return "FINISH";
  case kml::BookmarkIcon::Count: CHECK(false, ("Unknown bookmark icon")); return {};
  }
}

std::string ColorDataToString(kml::ColorData const & c)
{
  std::ostringstream out;
  out << "["
      << "predefined_color:" << PredefinedColorToString(c.m_predefinedColor) << ", "
      << "rgba:" << c.m_rgba << "]";
  return out.str();
}

std::string LatLonToString(ms::LatLon const & latLon)
{
  std::ostringstream out;
  out << "["
      << "lat:" << latLon.m_lat << ", "
      << "lon:" << latLon.m_lon << "]";
  return out.str();
}

std::string CompilationTypeToString(kml::CompilationType compilationType)
{
  switch (compilationType)
  {
  case kml::CompilationType::Category: return "Category";
  case kml::CompilationType::Collection: return "Collection";
  case kml::CompilationType::Day: return "Day";
  case kml::CompilationType::Count: CHECK(false, ("Unknown access rules")); return {};
  }
}

std::string BookmarkDataToString(kml::BookmarkData const & bm)
{
  std::ostringstream out;
  ms::LatLon const latLon(mercator::YToLat(bm.m_point.y), mercator::XToLon(bm.m_point.x));
  out << "["
      << "name:" << LocalizableStringAdapter::ToString(bm.m_name) << ", "
      << "description:" << LocalizableStringAdapter::ToString(bm.m_description) << ", "
      << "feature_types:" << VectorAdapter<uint32_t>::ToString(bm.m_featureTypes) << ", "
      << "custom_name:" << LocalizableStringAdapter::ToString(bm.m_customName) << ", "
      << "color:" << ColorDataToString(bm.m_color) << ", "
      << "icon:" << BookmarkIconToString(bm.m_icon) << ", "
      << "viewport_scale:" << static_cast<uint32_t>(bm.m_viewportScale) << ", "
      << "timestamp:" << kml::DebugPrint(bm.m_timestamp) << ", "
      << "point:" << LatLonToString(latLon) << ", "
      << "bound_tracks:" << VectorAdapter<uint8_t>::ToString(bm.m_boundTracks) << ", "
      << "visible:" << (bm.m_visible ? "True" : "False") << ", "
      << "nearest_toponym:'" << bm.m_nearestToponym << "', "
      << "compilations:" << VectorAdapter<uint64_t>::ToString(bm.m_compilations) << ", "
      << "properties:" << PropertiesAdapter::ToString(bm.m_properties) << "]";
  return out.str();
}

std::string TrackLayerToString(kml::TrackLayer const & trackLayer)
{
  std::ostringstream out;
  out << "["
      << "line_width:" << trackLayer.m_lineWidth << ", "
      << "color:" << ColorDataToString(trackLayer.m_color) << "]";
  return out.str();
}

std::string TrackDataToString(kml::TrackData const & t)
{
  std::ostringstream out;
  out << "["
      << "local_id:" << static_cast<uint32_t>(t.m_localId) << ", "
      << "name:" << LocalizableStringAdapter::ToString(t.m_name) << ", "
      << "description:" << LocalizableStringAdapter::ToString(t.m_description) << ", "
      << "timestamp:" << kml::DebugPrint(t.m_timestamp) << ", "
      << "layers:" << VectorAdapter<kml::TrackLayer>::ToString(t.m_layers) << ", "
      << "points_with_altitudes:" << VectorAdapter<geometry::PointWithAltitude>::ToString(t.m_pointsWithAltitudes)
      << ", "
      << "visible:" << (t.m_visible ? "True" : "False") << ", "
      << "nearest_toponyms:" << VectorAdapter<std::string>::ToString(t.m_nearestToponyms) << ", "
      << "properties:" << PropertiesAdapter::ToString(t.m_properties) << "]";
  return out.str();
}

std::string LanguagesListToString(std::vector<int8_t> const & langs)
{
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < langs.size(); ++i)
  {
    out << "'" << StringUtf8Multilang::GetLangByCode(langs[i]) << "'";
    if (i + 1 != langs.size())
      out << ", ";
  }
  out << "]";
  return out.str();
}

std::string CategoryDataToString(kml::CategoryData const & c)
{
  std::ostringstream out;
  out << "["
      << "type:" << CompilationTypeToString(c.m_type) << ", "
      << "compilation_id:" << c.m_compilationId << ", "
      << "name:" << LocalizableStringAdapter::ToString(c.m_name) << ", "
      << "annotation:" << LocalizableStringAdapter::ToString(c.m_annotation) << ", "
      << "description:" << LocalizableStringAdapter::ToString(c.m_description) << ", "
      << "image_url:'" << c.m_imageUrl << "', "
      << "visible:" << (c.m_visible ? "True" : "False") << ", "
      << "author_name:'" << c.m_authorName << "', "
      << "author_id:'" << c.m_authorId << "', "
      << "last_modified:" << kml::DebugPrint(c.m_lastModified) << ", "
      << "rating:" << c.m_rating << ", "
      << "reviews_number:" << c.m_reviewsNumber << ", "
      << "access_rules:" << AccessRulesToString(c.m_accessRules) << ", "
      << "tags:" << VectorAdapter<std::string>::ToString(c.m_tags) << ", "
      << "toponyms:" << VectorAdapter<std::string>::ToString(c.m_toponyms) << ", "
      << "languages:" << LanguagesListToString(c.m_languageCodes) << ", "
      << "properties:" << PropertiesAdapter::ToString(c.m_properties) << "]";
  return out.str();
}

std::string FileDataToString(kml::FileData const & fd)
{
  std::ostringstream out;
  out << "["
      << "server_id:" << fd.m_serverId << ", "
      << "category:" << CategoryDataToString(fd.m_categoryData) << ", "
      << "bookmarks:" << VectorAdapter<kml::BookmarkData>::ToString(fd.m_bookmarksData) << ", "
      << "tracks:" << VectorAdapter<kml::TrackData>::ToString(fd.m_tracksData) << ", "
      << "compilations:" << VectorAdapter<kml::CategoryData>::ToString(fd.m_compilationsData) << "]";
  return out.str();
}

struct TimestampConverter
{
  TimestampConverter()
  {
    boost::python::converter::registry::push_back(&convertible, &construct, boost::python::type_id<kml::Timestamp>());
  }

  static void * convertible(PyObject * objPtr)
  {
    boost::python::extract<uint64_t> checker(objPtr);
    if (!checker.check())
      return nullptr;
    return objPtr;
  }

  static void construct(PyObject * objPtr, boost::python::converter::rvalue_from_python_stage1_data * data)
  {
    auto const ts = kml::FromSecondsSinceEpoch(boost::python::extract<uint64_t>(objPtr));
    void * storage =
        reinterpret_cast<boost::python::converter::rvalue_from_python_storage<kml::Timestamp> *>(data)->storage.bytes;
    new (storage) kml::Timestamp(ts);
    data->convertible = storage;
  }
};

struct LatLonConverter
{
  LatLonConverter()
  {
    boost::python::converter::registry::push_back(&convertible, &construct, boost::python::type_id<m2::PointD>());
  }

  static void * convertible(PyObject * objPtr)
  {
    boost::python::extract<ms::LatLon> checker(objPtr);
    if (!checker.check())
      return nullptr;
    return objPtr;
  }

  static void construct(PyObject * objPtr, boost::python::converter::rvalue_from_python_stage1_data * data)
  {
    ms::LatLon latLon = boost::python::extract<ms::LatLon>(objPtr);
    m2::PointD pt(mercator::LonToX(latLon.m_lon), mercator::LatToY(latLon.m_lat));
    void * storage =
        reinterpret_cast<boost::python::converter::rvalue_from_python_storage<m2::PointD> *>(data)->storage.bytes;
    new (storage) m2::PointD(pt);
    data->convertible = storage;
  }
};

void TranslateRuntimeError(std::runtime_error const & e)
{
  PyErr_SetString(PyExc_RuntimeError, e.what());
}

boost::python::list GetLanguages(std::vector<int8_t> const & langs)
{
  std::vector<std::string> result;
  result.reserve(langs.size());
  for (auto const & langCode : langs)
  {
    std::string lang = StringUtf8Multilang::GetLangByCode(langCode);
    if (lang.empty())
      throw std::runtime_error("Language not found. langCode: " + std::to_string(langCode));
    result.emplace_back(std::move(lang));
  }
  return pyhelpers::StdVectorToPythonList(result);
}

void SetLanguages(std::vector<int8_t> & langs, boost::python::object const & iterable)
{
  langs.clear();
  if (iterable.is_none())
    return;

  auto const langStr = pyhelpers::PythonListToStdVector<std::string>(iterable);
  langs.reserve(langStr.size());
  for (auto const & lang : langStr)
  {
    auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
    if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
      throw std::runtime_error("Unsupported language. lang: " + lang);
    langs.emplace_back(langIndex);
  }
}

boost::python::list GetSupportedLanguages()
{
  auto const & supportedLangs = StringUtf8Multilang::GetSupportedLanguages();
  std::vector<std::string> langs;
  langs.reserve(supportedLangs.size());
  for (auto const & lang : supportedLangs)
    langs.emplace_back(lang.m_code);
  return pyhelpers::StdVectorToPythonList(langs);
}

boost::python::object GetLanguageIndex(std::string const & lang)
{
  auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
  if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
    throw std::runtime_error("Unsupported language. lang: " + lang);
  return boost::python::object(langIndex);
}

std::string ExportKml(kml::FileData const & fileData)
{
  std::string resultBuffer;
  try
  {
    MemWriter<decltype(resultBuffer)> sink(resultBuffer);
    kml::SerializerKml ser(fileData);
    ser.Serialize(sink);
  }
  catch (kml::SerializerKml::SerializeException const & exc)
  {
    throw std::runtime_error(std::string("Export error: ") + exc.what());
  }
  return resultBuffer;
}

kml::FileData ImportKml(std::string const & str)
{
  kml::FileData data;
  try
  {
    kml::DeserializerKml des(data);
    MemReader reader(str.data(), str.size());
    des.Deserialize(reader);
  }
  catch (kml::DeserializerKml::DeserializeException const & exc)
  {
    throw std::runtime_error(std::string("Import error: ") + exc.what());
  }
  return data;
}

void LoadClassificatorTypes(std::string const & classificatorFileStr, std::string const & typesFileStr)
{
  classificator::LoadTypes(classificatorFileStr, typesFileStr);
}

uint32_t ClassificatorTypeToIndex(std::string const & typeStr)
{
  if (typeStr.empty())
    throw std::runtime_error("Empty type is not allowed.");

  auto const & c = classif();
  if (!c.HasTypesMapping())
    throw std::runtime_error("Types mapping is not loaded. typeStr: " + typeStr);

  auto const type = c.GetTypeByReadableObjectName(typeStr);
  if (!c.IsTypeValid(type))
    throw std::runtime_error("Type is not valid. typeStr: " + typeStr);

  return c.GetIndexForType(type);
}

std::string IndexToClassificatorType(uint32_t index)
{
  auto const & c = classif();
  if (!c.HasTypesMapping())
    throw std::runtime_error("Types mapping is not loaded.");

  uint32_t t;
  try
  {
    t = c.GetTypeForIndex(index);
  }
  catch (std::out_of_range const & exc)
  {
    throw std::runtime_error("Type is not found. index: " + std::to_string(index));
  }

  if (!c.IsTypeValid(t))
    throw std::runtime_error("Type is not valid. type: " + std::to_string(t));

  return c.GetReadableObjectName(t);
}
}  // namespace

BOOST_PYTHON_MODULE(pykmlib)
{
  boost::python::scope().attr("__version__") = PYBINDINGS_VERSION;
  boost::python::scope().attr("invalid_altitude") = geometry::kInvalidAltitude;
  boost::python::register_exception_translator<std::runtime_error>(&TranslateRuntimeError);
  TimestampConverter();
  LatLonConverter();

  boost::python::enum_<kml::PredefinedColor>("PredefinedColor")
      .value(PredefinedColorToString(kml::PredefinedColor::None).c_str(), kml::PredefinedColor::None)
      .value(PredefinedColorToString(kml::PredefinedColor::Red).c_str(), kml::PredefinedColor::Red)
      .value(PredefinedColorToString(kml::PredefinedColor::Blue).c_str(), kml::PredefinedColor::Blue)
      .value(PredefinedColorToString(kml::PredefinedColor::Purple).c_str(), kml::PredefinedColor::Purple)
      .value(PredefinedColorToString(kml::PredefinedColor::Yellow).c_str(), kml::PredefinedColor::Yellow)
      .value(PredefinedColorToString(kml::PredefinedColor::Pink).c_str(), kml::PredefinedColor::Pink)
      .value(PredefinedColorToString(kml::PredefinedColor::Brown).c_str(), kml::PredefinedColor::Brown)
      .value(PredefinedColorToString(kml::PredefinedColor::Green).c_str(), kml::PredefinedColor::Green)
      .value(PredefinedColorToString(kml::PredefinedColor::Orange).c_str(), kml::PredefinedColor::Orange)
      .value(PredefinedColorToString(kml::PredefinedColor::DeepPurple).c_str(), kml::PredefinedColor::DeepPurple)
      .value(PredefinedColorToString(kml::PredefinedColor::LightBlue).c_str(), kml::PredefinedColor::LightBlue)
      .value(PredefinedColorToString(kml::PredefinedColor::Cyan).c_str(), kml::PredefinedColor::Cyan)
      .value(PredefinedColorToString(kml::PredefinedColor::Teal).c_str(), kml::PredefinedColor::Teal)
      .value(PredefinedColorToString(kml::PredefinedColor::Lime).c_str(), kml::PredefinedColor::Lime)
      .value(PredefinedColorToString(kml::PredefinedColor::DeepOrange).c_str(), kml::PredefinedColor::DeepOrange)
      .value(PredefinedColorToString(kml::PredefinedColor::Gray).c_str(), kml::PredefinedColor::Gray)
      .value(PredefinedColorToString(kml::PredefinedColor::BlueGray).c_str(), kml::PredefinedColor::BlueGray)
      .export_values();

  boost::python::enum_<kml::AccessRules>("AccessRules")
      .value(AccessRulesToString(kml::AccessRules::Local).c_str(), kml::AccessRules::Local)
      .value(AccessRulesToString(kml::AccessRules::DirectLink).c_str(), kml::AccessRules::DirectLink)
      .value(AccessRulesToString(kml::AccessRules::P2P).c_str(), kml::AccessRules::P2P)
      .value(AccessRulesToString(kml::AccessRules::Paid).c_str(), kml::AccessRules::Paid)
      .value(AccessRulesToString(kml::AccessRules::Public).c_str(), kml::AccessRules::Public)
      .value(AccessRulesToString(kml::AccessRules::AuthorOnly).c_str(), kml::AccessRules::AuthorOnly)
      .export_values();

  boost::python::enum_<kml::BookmarkIcon>("BookmarkIcon")
      .value(BookmarkIconToString(kml::BookmarkIcon::None).c_str(), kml::BookmarkIcon::None)
      .value(BookmarkIconToString(kml::BookmarkIcon::Hotel).c_str(), kml::BookmarkIcon::Hotel)
      .value(BookmarkIconToString(kml::BookmarkIcon::Animals).c_str(), kml::BookmarkIcon::Animals)
      .value(BookmarkIconToString(kml::BookmarkIcon::Buddhism).c_str(), kml::BookmarkIcon::Buddhism)
      .value(BookmarkIconToString(kml::BookmarkIcon::Building).c_str(), kml::BookmarkIcon::Building)
      .value(BookmarkIconToString(kml::BookmarkIcon::Christianity).c_str(), kml::BookmarkIcon::Christianity)
      .value(BookmarkIconToString(kml::BookmarkIcon::Entertainment).c_str(), kml::BookmarkIcon::Entertainment)
      .value(BookmarkIconToString(kml::BookmarkIcon::Exchange).c_str(), kml::BookmarkIcon::Exchange)
      .value(BookmarkIconToString(kml::BookmarkIcon::Food).c_str(), kml::BookmarkIcon::Food)
      .value(BookmarkIconToString(kml::BookmarkIcon::Gas).c_str(), kml::BookmarkIcon::Gas)
      .value(BookmarkIconToString(kml::BookmarkIcon::Judaism).c_str(), kml::BookmarkIcon::Judaism)
      .value(BookmarkIconToString(kml::BookmarkIcon::Medicine).c_str(), kml::BookmarkIcon::Medicine)
      .value(BookmarkIconToString(kml::BookmarkIcon::Mountain).c_str(), kml::BookmarkIcon::Mountain)
      .value(BookmarkIconToString(kml::BookmarkIcon::Museum).c_str(), kml::BookmarkIcon::Museum)
      .value(BookmarkIconToString(kml::BookmarkIcon::Islam).c_str(), kml::BookmarkIcon::Islam)
      .value(BookmarkIconToString(kml::BookmarkIcon::Park).c_str(), kml::BookmarkIcon::Park)
      .value(BookmarkIconToString(kml::BookmarkIcon::Parking).c_str(), kml::BookmarkIcon::Parking)
      .value(BookmarkIconToString(kml::BookmarkIcon::Shop).c_str(), kml::BookmarkIcon::Shop)
      .value(BookmarkIconToString(kml::BookmarkIcon::Sights).c_str(), kml::BookmarkIcon::Sights)
      .value(BookmarkIconToString(kml::BookmarkIcon::Swim).c_str(), kml::BookmarkIcon::Swim)
      .value(BookmarkIconToString(kml::BookmarkIcon::Water).c_str(), kml::BookmarkIcon::Water)
      .value(BookmarkIconToString(kml::BookmarkIcon::Bar).c_str(), kml::BookmarkIcon::Bar)
      .value(BookmarkIconToString(kml::BookmarkIcon::Transport).c_str(), kml::BookmarkIcon::Transport)
      .value(BookmarkIconToString(kml::BookmarkIcon::Viewpoint).c_str(), kml::BookmarkIcon::Viewpoint)
      .value(BookmarkIconToString(kml::BookmarkIcon::Sport).c_str(), kml::BookmarkIcon::Sport)
      .value(BookmarkIconToString(kml::BookmarkIcon::Start).c_str(), kml::BookmarkIcon::Start)
      .value(BookmarkIconToString(kml::BookmarkIcon::Finish).c_str(), kml::BookmarkIcon::Finish)
      .export_values();

  boost::python::enum_<kml::CompilationType>("CompilationType")
      .value(CompilationTypeToString(kml::CompilationType::Category).c_str(), kml::CompilationType::Category)
      .value(CompilationTypeToString(kml::CompilationType::Collection).c_str(), kml::CompilationType::Collection)
      .value(CompilationTypeToString(kml::CompilationType::Day).c_str(), kml::CompilationType::Day)
      .export_values();

  boost::python::class_<kml::ColorData>("ColorData")
      .def_readwrite("predefined_color", &kml::ColorData::m_predefinedColor)
      .def_readwrite("rgba", &kml::ColorData::m_rgba)
      .def("__eq__", &kml::ColorData::operator==)
      .def("__ne__", &kml::ColorData::operator!=)
      .def("__str__", &ColorDataToString);

  boost::python::class_<kml::LocalizableString>("LocalizableString")
      .def("__len__", &kml::LocalizableString::size)
      .def("clear", &kml::LocalizableString::clear)
      .def("__getitem__", &LocalizableStringAdapter::Get,
           boost::python::return_value_policy<boost::python::copy_const_reference>())
      .def("__setitem__", &LocalizableStringAdapter::Set, boost::python::with_custodian_and_ward<1, 2>())
      .def("__delitem__", &LocalizableStringAdapter::Delete)
      .def("get_dict", &LocalizableStringAdapter::GetDict)
      .def("set_dict", &LocalizableStringAdapter::SetDict)
      .def("__str__", &LocalizableStringAdapter::ToString);

  boost::python::class_<std::vector<std::string>>("StringList")
      .def(boost::python::vector_indexing_suite<std::vector<std::string>>())
      .def("get_list", &VectorAdapter<std::string>::Get)
      .def("set_list", &VectorAdapter<std::string>::Set)
      .def("__str__", &VectorAdapter<std::string>::ToString);

  boost::python::class_<std::vector<uint64_t>>("Uint64List")
      .def(boost::python::vector_indexing_suite<std::vector<uint64_t>>())
      .def("get_list", &VectorAdapter<uint64_t>::Get)
      .def("set_list", &VectorAdapter<uint64_t>::Set)
      .def("__str__", &VectorAdapter<uint64_t>::ToString);

  boost::python::class_<std::vector<uint32_t>>("Uint32List")
      .def(boost::python::vector_indexing_suite<std::vector<uint32_t>>())
      .def("get_list", &VectorAdapter<uint32_t>::Get)
      .def("set_list", &VectorAdapter<uint32_t>::Set)
      .def("__str__", &VectorAdapter<uint32_t>::ToString);

  boost::python::class_<std::vector<uint8_t>>("Uint8List")
      .def(boost::python::vector_indexing_suite<std::vector<uint8_t>>())
      .def("get_list", &VectorAdapter<uint8_t>::Get)
      .def("set_list", &VectorAdapter<uint8_t>::Set)
      .def("__str__", &VectorAdapter<uint8_t>::ToString);

  boost::python::class_<ms::LatLon>("LatLon", boost::python::init<double, double>())
      .def_readwrite("lat", &ms::LatLon::m_lat)
      .def_readwrite("lon", &ms::LatLon::m_lon)
      .def("__str__", &LatLonToString);

  boost::python::class_<geometry::PointWithAltitude>("PointWithAltitude")
      .def("get_point", &PointWithAltitudeAdapter::GetPoint,
           boost::python::return_value_policy<boost::python::copy_const_reference>())
      .def("set_point", &PointWithAltitudeAdapter::SetPoint)
      .def("get_altitude", &PointWithAltitudeAdapter::GetAltitude)
      .def("set_altitude", &PointWithAltitudeAdapter::SetAltitude)
      .def("__str__", &PointWithAltitudeAdapter::ToString);

  boost::python::class_<m2::PointD>("PointD");

  boost::python::class_<kml::Timestamp>("Timestamp");

  boost::python::class_<kml::Properties>("Properties")
      .def("__len__", &kml::Properties::size)
      .def("clear", &kml::Properties::clear)
      .def("__getitem__", &PropertiesAdapter::Get,
           boost::python::return_value_policy<boost::python::copy_const_reference>())
      .def("__setitem__", &PropertiesAdapter::Set, boost::python::with_custodian_and_ward<1, 2>())
      .def("__delitem__", &PropertiesAdapter::Delete)
      .def("get_dict", &PropertiesAdapter::GetDict)
      .def("set_dict", &PropertiesAdapter::SetDict)
      .def("__str__", &PropertiesAdapter::ToString);

  boost::python::class_<kml::BookmarkData>("BookmarkData")
      .def_readwrite("name", &kml::BookmarkData::m_name)
      .def_readwrite("description", &kml::BookmarkData::m_description)
      .def_readwrite("feature_types", &kml::BookmarkData::m_featureTypes)
      .def_readwrite("custom_name", &kml::BookmarkData::m_customName)
      .def_readwrite("color", &kml::BookmarkData::m_color)
      .def_readwrite("icon", &kml::BookmarkData::m_icon)
      .def_readwrite("viewport_scale", &kml::BookmarkData::m_viewportScale)
      .def_readwrite("timestamp", &kml::BookmarkData::m_timestamp)
      .def_readwrite("point", &kml::BookmarkData::m_point)
      .def_readwrite("bound_tracks", &kml::BookmarkData::m_boundTracks)
      .def_readwrite("visible", &kml::BookmarkData::m_visible)
      .def_readwrite("nearest_toponym", &kml::BookmarkData::m_nearestToponym)
      .def_readwrite("compilations", &kml::BookmarkData::m_compilations)
      .def_readwrite("properties", &kml::BookmarkData::m_properties)
      .def("__eq__", &kml::BookmarkData::operator==)
      .def("__ne__", &kml::BookmarkData::operator!=)
      .def("__str__", &BookmarkDataToString);

  boost::python::class_<kml::TrackLayer>("TrackLayer")
      .def_readwrite("line_width", &kml::TrackLayer::m_lineWidth)
      .def_readwrite("color", &kml::TrackLayer::m_color)
      .def("__eq__", &kml::TrackLayer::operator==)
      .def("__ne__", &kml::TrackLayer::operator!=)
      .def("__str__", &TrackLayerToString);

  boost::python::class_<std::vector<kml::TrackLayer>>("TrackLayerList")
      .def(boost::python::vector_indexing_suite<std::vector<kml::TrackLayer>>())
      .def("get_list", &VectorAdapter<kml::TrackLayer>::Get)
      .def("set_list", &VectorAdapter<kml::TrackLayer>::Set)
      .def("__str__", &VectorAdapter<kml::TrackLayer>::ToString);

  boost::python::class_<std::vector<m2::PointD>>("PointDList")
      .def(boost::python::vector_indexing_suite<std::vector<m2::PointD>>())
      .def("get_list", &VectorAdapter<m2::PointD>::Get)
      .def("set_list", &VectorAdapter<m2::PointD>::Set)
      .def("__str__", &VectorAdapter<m2::PointD>::ToString);

  boost::python::class_<std::vector<geometry::PointWithAltitude>>("PointWithAltitudeList")
      .def(boost::python::vector_indexing_suite<std::vector<geometry::PointWithAltitude>>())
      .def("get_list", &VectorAdapter<geometry::PointWithAltitude>::Get)
      .def("set_list", &VectorAdapter<geometry::PointWithAltitude>::Set)
      .def("__str__", &VectorAdapter<geometry::PointWithAltitude>::ToString);

  boost::python::class_<std::vector<ms::LatLon>>("LatLonList")
      .def(boost::python::vector_indexing_suite<std::vector<ms::LatLon>>());

  boost::python::class_<kml::TrackData>("TrackData")
      .def_readwrite("local_id", &kml::TrackData::m_localId)
      .def_readwrite("name", &kml::TrackData::m_name)
      .def_readwrite("description", &kml::TrackData::m_description)
      .def_readwrite("timestamp", &kml::TrackData::m_timestamp)
      .def_readwrite("layers", &kml::TrackData::m_layers)
      .def_readwrite("points_with_altitudes", &kml::TrackData::m_pointsWithAltitudes)
      .def_readwrite("visible", &kml::TrackData::m_visible)
      .def_readwrite("nearest_toponyms", &kml::TrackData::m_nearestToponyms)
      .def_readwrite("properties", &kml::TrackData::m_properties)
      .def("__eq__", &kml::TrackData::operator==)
      .def("__ne__", &kml::TrackData::operator!=)
      .def("__str__", &TrackDataToString);

  boost::python::class_<std::vector<int8_t>>("LanguageList")
      .def(boost::python::vector_indexing_suite<std::vector<int8_t>>())
      .def("get_list", &GetLanguages)
      .def("set_list", &SetLanguages)
      .def("__str__", &LanguagesListToString);

  boost::python::class_<kml::CategoryData>("CategoryData")
      .def_readwrite("type", &kml::CategoryData::m_type)
      .def_readwrite("compilation_id", &kml::CategoryData::m_compilationId)
      .def_readwrite("name", &kml::CategoryData::m_name)
      .def_readwrite("annotation", &kml::CategoryData::m_annotation)
      .def_readwrite("description", &kml::CategoryData::m_description)
      .def_readwrite("image_url", &kml::CategoryData::m_imageUrl)
      .def_readwrite("visible", &kml::CategoryData::m_visible)
      .def_readwrite("author_name", &kml::CategoryData::m_authorName)
      .def_readwrite("author_id", &kml::CategoryData::m_authorId)
      .def_readwrite("last_modified", &kml::CategoryData::m_lastModified)
      .def_readwrite("rating", &kml::CategoryData::m_rating)
      .def_readwrite("reviews_number", &kml::CategoryData::m_reviewsNumber)
      .def_readwrite("access_rules", &kml::CategoryData::m_accessRules)
      .def_readwrite("tags", &kml::CategoryData::m_tags)
      .def_readwrite("toponyms", &kml::CategoryData::m_toponyms)
      .def_readwrite("languages", &kml::CategoryData::m_languageCodes)
      .def_readwrite("properties", &kml::CategoryData::m_properties)
      .def("__eq__", &kml::CategoryData::operator==)
      .def("__ne__", &kml::CategoryData::operator!=)
      .def("__str__", &CategoryDataToString);

  boost::python::class_<std::vector<kml::BookmarkData>>("BookmarkList")
      .def(boost::python::vector_indexing_suite<std::vector<kml::BookmarkData>>())
      .def("get_list", &VectorAdapter<kml::BookmarkData>::Get)
      .def("set_list", &VectorAdapter<kml::BookmarkData>::Set)
      .def("__str__", &VectorAdapter<kml::BookmarkData>::ToString);

  boost::python::class_<std::vector<kml::TrackData>>("TrackList")
      .def(boost::python::vector_indexing_suite<std::vector<kml::TrackData>>())
      .def("get_list", &VectorAdapter<kml::TrackData>::Get)
      .def("set_list", &VectorAdapter<kml::TrackData>::Set)
      .def("__str__", &VectorAdapter<kml::TrackData>::ToString);

  boost::python::class_<std::vector<kml::CategoryData>>("CompilationList")
      .def(boost::python::vector_indexing_suite<std::vector<kml::CategoryData>>())
      .def("get_list", &VectorAdapter<kml::CategoryData>::Get)
      .def("set_list", &VectorAdapter<kml::CategoryData>::Set)
      .def("__str__", &VectorAdapter<kml::CategoryData>::ToString);

  boost::python::class_<kml::FileData>("FileData")
      .def_readwrite("server_id", &kml::FileData::m_serverId)
      .def_readwrite("category", &kml::FileData::m_categoryData)
      .def_readwrite("bookmarks", &kml::FileData::m_bookmarksData)
      .def_readwrite("tracks", &kml::FileData::m_tracksData)
      .def_readwrite("compilations", &kml::FileData::m_compilationsData)
      .def("__eq__", &kml::FileData::operator==)
      .def("__ne__", &kml::FileData::operator!=)
      .def("__str__", &FileDataToString);

  boost::python::def("set_bookmarks_min_zoom", &kml::SetBookmarksMinZoom);

  boost::python::def("get_supported_languages", GetSupportedLanguages);
  boost::python::def("get_language_index", GetLanguageIndex);
  boost::python::def("timestamp_to_int", &kml::ToSecondsSinceEpoch);
  boost::python::def("point_to_latlon", &mercator::ToLatLon);

  boost::python::def("export_kml", ExportKml);
  boost::python::def("import_kml", ImportKml);

  boost::python::def("load_classificator_types", LoadClassificatorTypes);
  boost::python::def("classificator_type_to_index", ClassificatorTypeToIndex);
  boost::python::def("index_to_classificator_type", IndexToClassificatorType);
}
