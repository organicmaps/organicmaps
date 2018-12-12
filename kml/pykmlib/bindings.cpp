#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

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

#include "pyhelpers/vector_uint8.hpp"
#include "pyhelpers/vector_list_conversion.hpp"

#include <boost/python.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#pragma GCC diagnostic pop

using namespace kml;
using namespace boost::python;

namespace
{
struct LocalizableStringAdapter
{
  static std::string const & Get(LocalizableString const & str, std::string const & lang)
  {
    auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
    auto const it = str.find(langIndex);
    if (it != str.end())
      return it->second;
    throw std::runtime_error("Language not found");
  }

  static void Set(LocalizableString & str, std::string const & lang, std::string const & val)
  {
    auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
    if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
      throw std::runtime_error("Unsupported language");
    str[langIndex] = val;
  }

  static void Delete(LocalizableString & str, std::string const & lang)
  {
    auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
    auto const it = str.find(langIndex);
    if (it != str.end())
      str.erase(it);
    else
      throw std::runtime_error("Language not found");
  }

  static boost::python::dict GetDict(LocalizableString const & str)
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

  static void SetDict(LocalizableString & str, boost::python::dict & dict)
  {
    str.clear();
    if (dict.is_none())
      return;

    auto const langs = python_list_to_std_vector<std::string>(dict.keys());
    for (auto const & lang : langs)
    {
      auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
      if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
        throw std::runtime_error("Unsupported language");

      str[langIndex] = extract<std::string>(dict[lang]);
    }
  }

  static std::string ToString(LocalizableString const & str)
  {
    std::ostringstream out;
    out << "[";
    for (auto it = str.begin(); it != str.end(); ++it)
    {
      out << "'"<< StringUtf8Multilang::GetLangByCode(it->first) << "':'" << it->second << "'";
      auto it2 = it;
      it2++;
      if (it2 != str.end())
        out << ", ";
    }
    out << "]";
    return out.str();
  }
};

struct PropertiesAdapter
{
  static std::string const & Get(Properties const & props, std::string const & key)
  {
    auto const it = props.find(key);
    if (it != props.end())
      return it->second;
    throw std::runtime_error("Property not found");
  }

  static void Set(Properties & props, std::string const & key, std::string const & val)
  {
    props[key] = val;
  }

  static void Delete(Properties & props, std::string const & key)
  {
    auto const it = props.find(key);
    if (it != props.end())
      props.erase(it);
    else
      throw std::runtime_error("Property not found");
  }

  static boost::python::dict GetDict(Properties const & props)
  {
    boost::python::dict d;
    for (auto const & p : props)
      d[p.first] = p.second;
    return d;
  }

  static void SetDict(Properties & props, boost::python::dict & dict)
  {
    props.clear();
    if (dict.is_none())
      return;

    auto const keys = python_list_to_std_vector<std::string>(dict.keys());
    for (auto const & k : keys)
      props[k] = extract<std::string>(dict[k]);
  }

  static std::string ToString(Properties const & props)
  {
    std::ostringstream out;
    out << "[";
    for (auto it = props.begin(); it != props.end(); ++it)
    {
      out << "'"<< it->first << "':'" << it->second << "'";
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
  static boost::python::list Get(std::vector<T> const & v)
  {
    return std_vector_to_python_list(v);
  }

  static void Set(std::vector<T> & v, boost::python::object const & iterable)
  {
    if (iterable.is_none())
    {
      v.clear();
      return;
    }
    v = python_list_to_std_vector<T>(iterable);
  }

  static void PrintType(std::ostringstream & out, T const & t)
  {
    out << t;
  }

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

template<>
void VectorAdapter<uint8_t>::PrintType(std::ostringstream & out, uint8_t const & t)
{
  out << static_cast<uint32_t>(t);
}

template<>
void VectorAdapter<std::string>::PrintType(std::ostringstream & out, std::string const & s)
{
  out << "'" << s << "'";
}

std::string TrackLayerToString(TrackLayer const & trackLayer);
template<>
void VectorAdapter<TrackLayer>::PrintType(std::ostringstream & out, TrackLayer const & t)
{
  out << TrackLayerToString(t);
}

template<>
boost::python::list VectorAdapter<m2::PointD>::Get(std::vector<m2::PointD> const & points)
{
  std::vector<ms::LatLon> latLonArray;
  latLonArray.reserve(points.size());
  for (size_t i = 0; i < points.size(); ++i)
    latLonArray.emplace_back(MercatorBounds::YToLat(points[i].y), MercatorBounds::XToLon(points[i].x));
  return std_vector_to_python_list(latLonArray);
}

template<>
void VectorAdapter<m2::PointD>::Set(std::vector<m2::PointD> & v, boost::python::object const & iterable)
{
  v.clear();
  if (iterable.is_none())
    return;

  auto const latLon = python_list_to_std_vector<ms::LatLon>(iterable);
  v.reserve(latLon.size());
  for (size_t i = 0; i < latLon.size(); ++i)
    v.emplace_back(MercatorBounds::LonToX(latLon[i].lon), MercatorBounds::LatToY(latLon[i].lat));
}

std::string LatLonToString(ms::LatLon const & latLon);
template<>
void VectorAdapter<m2::PointD>::PrintType(std::ostringstream & out, m2::PointD const & pt)
{
  ms::LatLon const latLon(MercatorBounds::YToLat(pt.y), MercatorBounds::XToLon(pt.x));
  out << LatLonToString(latLon);
}

std::string BookmarkDataToString(BookmarkData const & bm);
template<>
void VectorAdapter<BookmarkData>::PrintType(std::ostringstream & out, BookmarkData const & bm)
{
  out << BookmarkDataToString(bm);
}

std::string TrackDataToString(TrackData const & t);
template<>
void VectorAdapter<TrackData>::PrintType(std::ostringstream & out, TrackData const & t)
{
  out << TrackDataToString(t);
}

std::string PredefinedColorToString(PredefinedColor c)
{
  switch (c)
  {
  case PredefinedColor::None: return "NONE";
  case PredefinedColor::Red: return "RED";
  case PredefinedColor::Blue: return "BLUE";
  case PredefinedColor::Purple: return "PURPLE";
  case PredefinedColor::Yellow: return "YELLOW";
  case PredefinedColor::Pink: return "PINK";
  case PredefinedColor::Brown: return "BROWN";
  case PredefinedColor::Green: return "GREEN";
  case PredefinedColor::Orange: return "ORANGE";
  case PredefinedColor::Count: CHECK(false, ("Unknown predefined color")); return {};
  }
}

std::string AccessRulesToString(AccessRules accessRules)
{
  switch (accessRules)
  {
  case AccessRules::Local: return "LOCAL";
  case AccessRules::DirectLink: return "DIRECT_LINK";
  case AccessRules::P2P: return "P2P";
  case AccessRules::Paid: return "PAID";
  case AccessRules::Public: return "PUBLIC";
  case AccessRules::Count: CHECK(false, ("Unknown access rules")); return {};
  }
}

std::string BookmarkIconToString(BookmarkIcon icon)
{
  switch (icon)
  {
  case BookmarkIcon::None: return "NONE";
  case BookmarkIcon::Hotel: return "HOTEL";
  case BookmarkIcon::Animals: return "ANIMALS";
  case BookmarkIcon::Buddhism: return "BUDDHISM";
  case BookmarkIcon::Building: return "BUILDING";
  case BookmarkIcon::Christianity: return "CHRISTIANITY";
  case BookmarkIcon::Entertainment: return "ENTERTAINMENT";
  case BookmarkIcon::Exchange: return "EXCHANGE";
  case BookmarkIcon::Food: return "FOOD";
  case BookmarkIcon::Gas: return "GAS";
  case BookmarkIcon::Judaism: return "JUDAISM";
  case BookmarkIcon::Medicine: return "MEDICINE";
  case BookmarkIcon::Mountain: return "MOUNTAIN";
  case BookmarkIcon::Museum: return "MUSEUM";
  case BookmarkIcon::Islam: return "ISLAM";
  case BookmarkIcon::Park: return "PARK";
  case BookmarkIcon::Parking: return "PARKING";
  case BookmarkIcon::Shop: return "SHOP";
  case BookmarkIcon::Sights: return "SIGHTS";
  case BookmarkIcon::Swim: return "SWIM";
  case BookmarkIcon::Water: return "WATER";
  case BookmarkIcon::Count: CHECK(false, ("Unknown bookmark icon")); return {};
  }
}

std::string ColorDataToString(ColorData const & c)
{
  std::ostringstream out;
  out << "["
      << "predefined_color:"
      << PredefinedColorToString(c.m_predefinedColor)
      << ", "
      << "rgba:" << c.m_rgba << "]";
  return out.str();
}

std::string LatLonToString(ms::LatLon const & latLon)
{
  std::ostringstream out;
  out << "["
      << "lat:" << latLon.lat << ", "
      << "lon:" << latLon.lon
      << "]";
  return out.str();
}

std::string BookmarkDataToString(BookmarkData const & bm)
{
  std::ostringstream out;
  ms::LatLon const latLon(MercatorBounds::YToLat(bm.m_point.y), MercatorBounds::XToLon(bm.m_point.x));
  out << "["
      << "name:" << LocalizableStringAdapter::ToString(bm.m_name) << ", "
      << "description:" << LocalizableStringAdapter::ToString(bm.m_description) << ", "
      << "feature_types:" << VectorAdapter<uint32_t>::ToString(bm.m_featureTypes) << ", "
      << "custom_name:" << LocalizableStringAdapter::ToString(bm.m_customName) << ", "
      << "color:" << ColorDataToString(bm.m_color) << ", "
      << "icon:" << BookmarkIconToString(bm.m_icon) << ", "
      << "viewport_scale:" << static_cast<uint32_t>(bm.m_viewportScale) << ", "
      << "timestamp:" << DebugPrint(bm.m_timestamp) << ", "
      << "point:" << LatLonToString(latLon) << ", "
      << "bound_tracks:" << VectorAdapter<uint8_t>::ToString(bm.m_boundTracks)
      << "]";
  return out.str();
}

std::string TrackLayerToString(TrackLayer const & trackLayer)
{
  std::ostringstream out;
  out << "["
      << "line_width:" << trackLayer.m_lineWidth << ", "
      << "color:" << ColorDataToString(trackLayer.m_color)
      << "]";
  return out.str();
}

std::string TrackDataToString(TrackData const & t)
{
  std::ostringstream out;
  out << "["
      << "local_id:" << static_cast<uint32_t>(t.m_localId) << ", "
      << "name:" << LocalizableStringAdapter::ToString(t.m_name) << ", "
      << "description:" << LocalizableStringAdapter::ToString(t.m_description) << ", "
      << "timestamp:" << DebugPrint(t.m_timestamp) << ", "
      << "layers:" << VectorAdapter<TrackLayer>::ToString(t.m_layers) << ", "
      << "points:" << VectorAdapter<m2::PointD>::ToString(t.m_points)
      << "]";
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

std::string CategoryDataToString(CategoryData const & c)
{
  std::ostringstream out;
  out << "["
      << "name:" << LocalizableStringAdapter::ToString(c.m_name) << ", "
      << "annotation:" << LocalizableStringAdapter::ToString(c.m_annotation) << ", "
      << "description:" << LocalizableStringAdapter::ToString(c.m_description) << ", "
      << "image_url:'" << c.m_imageUrl << "', "
      << "visible:" << (c.m_visible ? "True" : "False") << ", "
      << "author_name:'" << c.m_authorName << "', "
      << "author_id:'" << c.m_authorId << "', "
      << "last_modified:" << DebugPrint(c.m_lastModified) << ", "
      << "rating:" << c.m_rating << ", "
      << "reviews_number:" << c.m_reviewsNumber << ", "
      << "access_rules:" << AccessRulesToString(c.m_accessRules) << ", "
      << "tags:" << VectorAdapter<std::string>::ToString(c.m_tags) << ", "
      << "cities:" << VectorAdapter<m2::PointD>::ToString(c.m_cities) << ", "
      << "languages:" << LanguagesListToString(c.m_languageCodes) << ", "
      << "properties:" << PropertiesAdapter::ToString(c.m_properties)
      << "]";
  return out.str();
}

std::string FileDataToString(FileData const & fd)
{
  std::ostringstream out;
  out << "["
      << "server_id:" << fd.m_serverId << ", "
      << "category:" << CategoryDataToString(fd.m_categoryData) << ", "
      << "bookmarks:" << VectorAdapter<BookmarkData>::ToString(fd.m_bookmarksData) << ", "
      << "tracks:" << VectorAdapter<TrackData>::ToString(fd.m_tracksData)
      << "]";
  return out.str();
}

struct TimestampConverter
{
  TimestampConverter()
  {
    converter::registry::push_back(&convertible, &construct, type_id<Timestamp>());
  }

  static void * convertible(PyObject * objPtr)
  {
    extract<uint64_t> checker(objPtr);
    if (!checker.check())
      return nullptr;
    return objPtr;
  }

  static void construct(PyObject * objPtr, converter::rvalue_from_python_stage1_data * data)
  {
    auto const ts = FromSecondsSinceEpoch(extract<uint64_t>(objPtr));
    void * storage =
      reinterpret_cast<converter::rvalue_from_python_storage<Timestamp> *>(data)->storage.bytes;
    new (storage) Timestamp(ts);
    data->convertible = storage;
  }
};

struct LatLonConverter
{
  LatLonConverter()
  {
    converter::registry::push_back(&convertible, &construct, type_id<m2::PointD>());
  }

  static void * convertible(PyObject * objPtr)
  {
    extract<ms::LatLon> checker(objPtr);
    if (!checker.check())
      return nullptr;
    return objPtr;
  }

  static void construct(PyObject * objPtr, converter::rvalue_from_python_stage1_data * data)
  {
    ms::LatLon latLon = extract<ms::LatLon>(objPtr);
    m2::PointD pt(MercatorBounds::LonToX(latLon.lon), MercatorBounds::LatToY(latLon.lat));
    void * storage =
      reinterpret_cast<converter::rvalue_from_python_storage<m2::PointD> *>(data)->storage.bytes;
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
      throw std::runtime_error("Language not found");
    result.emplace_back(std::move(lang));
  }
  return std_vector_to_python_list(result);
}

void SetLanguages(std::vector<int8_t> & langs, boost::python::object const & iterable)
{
  langs.clear();
  if (iterable.is_none())
    return;

  auto const langStr = python_list_to_std_vector<std::string>(iterable);
  langs.reserve(langStr.size());
  for (auto const & lang : langStr)
  {
    auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
    if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
      throw std::runtime_error("Unsupported language");
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
  return std_vector_to_python_list(langs);
}

boost::python::object GetLanguageIndex(std::string const & lang)
{
  auto const langIndex = StringUtf8Multilang::GetLangIndex(lang);
  if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
    throw std::runtime_error("Unsupported language");
  return boost::python::object(langIndex);
}

std::string ExportKml(FileData const & fileData)
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

FileData ImportKml(std::string const & str)
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

void LoadClassificatorTypes(std::string const & classificatorFileStr,
                            std::string const & typesFileStr)
{
  classificator::LoadTypes(classificatorFileStr, typesFileStr);
}

uint32_t ClassificatorTypeToIndex(std::string const & typeStr)
{
  if (typeStr.empty())
    throw std::runtime_error("Empty type is not allowed.");

  auto const & c = classif();
  if (!c.HasTypesMapping())
    throw std::runtime_error("Types mapping is not loaded.");

  auto const type = c.GetTypeByReadableObjectName(typeStr);
  if (!c.IsTypeValid(type))
    throw std::runtime_error("Type is not valid.");

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
    throw std::runtime_error("Type is not found.");
  }

  if (!c.IsTypeValid(t))
    throw std::runtime_error("Type is not valid.");

  return c.GetReadableObjectName(t);
}
}  // namespace

BOOST_PYTHON_MODULE(pykmlib)
{
  register_exception_translator<std::runtime_error>(&TranslateRuntimeError);
  TimestampConverter();
  LatLonConverter();

  enum_<PredefinedColor>("PredefinedColor")
    .value(PredefinedColorToString(PredefinedColor::None).c_str(), PredefinedColor::None)
    .value(PredefinedColorToString(PredefinedColor::Red).c_str(), PredefinedColor::Red)
    .value(PredefinedColorToString(PredefinedColor::Blue).c_str(), PredefinedColor::Blue)
    .value(PredefinedColorToString(PredefinedColor::Purple).c_str(), PredefinedColor::Purple)
    .value(PredefinedColorToString(PredefinedColor::Yellow).c_str(), PredefinedColor::Yellow)
    .value(PredefinedColorToString(PredefinedColor::Pink).c_str(), PredefinedColor::Pink)
    .value(PredefinedColorToString(PredefinedColor::Brown).c_str(), PredefinedColor::Brown)
    .value(PredefinedColorToString(PredefinedColor::Green).c_str(), PredefinedColor::Green)
    .value(PredefinedColorToString(PredefinedColor::Orange).c_str(), PredefinedColor::Orange)
    .export_values();

  enum_<AccessRules>("AccessRules")
    .value(AccessRulesToString(AccessRules::Local).c_str(), AccessRules::Local)
    .value(AccessRulesToString(AccessRules::DirectLink).c_str(), AccessRules::DirectLink)
    .value(AccessRulesToString(AccessRules::P2P).c_str(), AccessRules::P2P)
    .value(AccessRulesToString(AccessRules::Paid).c_str(), AccessRules::Paid)
    .value(AccessRulesToString(AccessRules::Public).c_str(), AccessRules::Public)
    .export_values();

  enum_<BookmarkIcon>("BookmarkIcon")
    .value(BookmarkIconToString(BookmarkIcon::None).c_str(), BookmarkIcon::None)
    .value(BookmarkIconToString(BookmarkIcon::Hotel).c_str(), BookmarkIcon::Hotel)
    .value(BookmarkIconToString(BookmarkIcon::Animals).c_str(), BookmarkIcon::Animals)
    .value(BookmarkIconToString(BookmarkIcon::Buddhism).c_str(), BookmarkIcon::Buddhism)
    .value(BookmarkIconToString(BookmarkIcon::Building).c_str(), BookmarkIcon::Building)
    .value(BookmarkIconToString(BookmarkIcon::Christianity).c_str(), BookmarkIcon::Christianity)
    .value(BookmarkIconToString(BookmarkIcon::Entertainment).c_str(), BookmarkIcon::Entertainment)
    .value(BookmarkIconToString(BookmarkIcon::Exchange).c_str(), BookmarkIcon::Exchange)
    .value(BookmarkIconToString(BookmarkIcon::Food).c_str(), BookmarkIcon::Food)
    .value(BookmarkIconToString(BookmarkIcon::Gas).c_str(), BookmarkIcon::Gas)
    .value(BookmarkIconToString(BookmarkIcon::Judaism).c_str(), BookmarkIcon::Judaism)
    .value(BookmarkIconToString(BookmarkIcon::Medicine).c_str(), BookmarkIcon::Medicine)
    .value(BookmarkIconToString(BookmarkIcon::Mountain).c_str(), BookmarkIcon::Mountain)
    .value(BookmarkIconToString(BookmarkIcon::Museum).c_str(), BookmarkIcon::Museum)
    .value(BookmarkIconToString(BookmarkIcon::Islam).c_str(), BookmarkIcon::Islam)
    .value(BookmarkIconToString(BookmarkIcon::Park).c_str(), BookmarkIcon::Park)
    .value(BookmarkIconToString(BookmarkIcon::Parking).c_str(), BookmarkIcon::Parking)
    .value(BookmarkIconToString(BookmarkIcon::Shop).c_str(), BookmarkIcon::Shop)
    .value(BookmarkIconToString(BookmarkIcon::Sights).c_str(), BookmarkIcon::Sights)
    .value(BookmarkIconToString(BookmarkIcon::Swim).c_str(), BookmarkIcon::Swim)
    .value(BookmarkIconToString(BookmarkIcon::Water).c_str(), BookmarkIcon::Water)
    .export_values();

  class_<ColorData>("ColorData")
    .def_readwrite("predefined_color", &ColorData::m_predefinedColor)
    .def_readwrite("rgba", &ColorData::m_rgba)
    .def("__eq__", &ColorData::operator==)
    .def("__ne__", &ColorData::operator!=)
    .def("__str__", &ColorDataToString);

  class_<LocalizableString>("LocalizableString")
    .def("__len__", &LocalizableString::size)
    .def("clear", &LocalizableString::clear)
    .def("__getitem__", &LocalizableStringAdapter::Get, return_value_policy<copy_const_reference>())
    .def("__setitem__", &LocalizableStringAdapter::Set, with_custodian_and_ward<1,2>())
    .def("__delitem__", &LocalizableStringAdapter::Delete)
    .def("get_dict", &LocalizableStringAdapter::GetDict)
    .def("set_dict", &LocalizableStringAdapter::SetDict)
    .def("__str__", &LocalizableStringAdapter::ToString);

  class_<std::vector<std::string>>("StringList")
    .def(vector_indexing_suite<std::vector<std::string>>())
    .def("get_list", &VectorAdapter<std::string>::Get)
    .def("set_list", &VectorAdapter<std::string>::Set)
    .def("__str__", &VectorAdapter<std::string>::ToString);

  class_<std::vector<uint32_t>>("Uint32List")
    .def(vector_indexing_suite<std::vector<uint32_t>>())
    .def("get_list", &VectorAdapter<uint32_t>::Get)
    .def("set_list", &VectorAdapter<uint32_t>::Set)
    .def("__str__", &VectorAdapter<uint32_t>::ToString);

  class_<std::vector<uint8_t>>("Uint8List")
    .def(vector_indexing_suite<std::vector<uint8_t>>())
    .def("get_list", &VectorAdapter<uint8_t>::Get)
    .def("set_list", &VectorAdapter<uint8_t>::Set)
    .def("__str__", &VectorAdapter<uint8_t>::ToString);

  class_<ms::LatLon>("LatLon", init<double, double>())
    .def_readwrite("lat", &ms::LatLon::lat)
    .def_readwrite("lon", &ms::LatLon::lon)
    .def("__str__", &LatLonToString);

  class_<m2::PointD>("PointD");

  class_<Timestamp>("Timestamp");

  class_<BookmarkData>("BookmarkData")
    .def_readwrite("name", &BookmarkData::m_name)
    .def_readwrite("description", &BookmarkData::m_description)
    .def_readwrite("feature_types", &BookmarkData::m_featureTypes)
    .def_readwrite("custom_name", &BookmarkData::m_customName)
    .def_readwrite("color", &BookmarkData::m_color)
    .def_readwrite("icon", &BookmarkData::m_icon)
    .def_readwrite("viewport_scale", &BookmarkData::m_viewportScale)
    .def_readwrite("timestamp", &BookmarkData::m_timestamp)
    .def_readwrite("point", &BookmarkData::m_point)
    .def_readwrite("bound_tracks", &BookmarkData::m_boundTracks)
    .def("__eq__", &BookmarkData::operator==)
    .def("__ne__", &BookmarkData::operator!=)
    .def("__str__", &BookmarkDataToString);

  class_<TrackLayer>("TrackLayer")
    .def_readwrite("line_width", &TrackLayer::m_lineWidth)
    .def_readwrite("color", &TrackLayer::m_color)
    .def("__eq__", &TrackLayer::operator==)
    .def("__ne__", &TrackLayer::operator!=)
    .def("__str__", &TrackLayerToString);

  class_<std::vector<TrackLayer>>("TrackLayerList")
    .def(vector_indexing_suite<std::vector<TrackLayer>>())
    .def("get_list", &VectorAdapter<TrackLayer>::Get)
    .def("set_list", &VectorAdapter<TrackLayer>::Set)
    .def("__str__", &VectorAdapter<TrackLayer>::ToString);

  class_<std::vector<m2::PointD>>("PointDList")
    .def(vector_indexing_suite<std::vector<m2::PointD>>())
    .def("get_list", &VectorAdapter<m2::PointD>::Get)
    .def("set_list", &VectorAdapter<m2::PointD>::Set)
    .def("__str__", &VectorAdapter<m2::PointD>::ToString);

  class_<std::vector<ms::LatLon>>("LatLonList")
    .def(vector_indexing_suite<std::vector<ms::LatLon>>());

  class_<TrackData>("TrackData")
    .def_readwrite("local_id", &TrackData::m_localId)
    .def_readwrite("name", &TrackData::m_name)
    .def_readwrite("description", &TrackData::m_description)
    .def_readwrite("timestamp", &TrackData::m_timestamp)
    .def_readwrite("layers", &TrackData::m_layers)
    .def_readwrite("points", &TrackData::m_points)
    .def("__eq__", &TrackData::operator==)
    .def("__ne__", &TrackData::operator!=)
    .def("__str__", &TrackDataToString);

  class_<std::vector<int8_t>>("LanguageList")
    .def(vector_indexing_suite<std::vector<int8_t>>())
    .def("get_list", &GetLanguages)
    .def("set_list", &SetLanguages)
    .def("__str__", &LanguagesListToString);

  class_<Properties>("Properties")
    .def("__len__", &Properties::size)
    .def("clear", &Properties::clear)
    .def("__getitem__", &PropertiesAdapter::Get, return_value_policy<copy_const_reference>())
    .def("__setitem__", &PropertiesAdapter::Set, with_custodian_and_ward<1,2>())
    .def("__delitem__", &PropertiesAdapter::Delete)
    .def("get_dict", &PropertiesAdapter::GetDict)
    .def("set_dict", &PropertiesAdapter::SetDict)
    .def("__str__", &PropertiesAdapter::ToString);

  class_<CategoryData>("CategoryData")
    .def_readwrite("name", &CategoryData::m_name)
    .def_readwrite("annotation", &CategoryData::m_annotation)
    .def_readwrite("description", &CategoryData::m_description)
    .def_readwrite("image_url", &CategoryData::m_imageUrl)
    .def_readwrite("visible", &CategoryData::m_visible)
    .def_readwrite("author_name", &CategoryData::m_authorName)
    .def_readwrite("author_id", &CategoryData::m_authorId)
    .def_readwrite("last_modified", &CategoryData::m_lastModified)
    .def_readwrite("rating", &CategoryData::m_rating)
    .def_readwrite("reviews_number", &CategoryData::m_reviewsNumber)
    .def_readwrite("access_rules", &CategoryData::m_accessRules)
    .def_readwrite("tags", &CategoryData::m_tags)
    .def_readwrite("cities", &CategoryData::m_cities)
    .def_readwrite("languages", &CategoryData::m_languageCodes)
    .def_readwrite("properties", &CategoryData::m_properties)
    .def("__eq__", &CategoryData::operator==)
    .def("__ne__", &CategoryData::operator!=)
    .def("__str__", &CategoryDataToString);

  class_<std::vector<BookmarkData>>("BookmarkList")
    .def(vector_indexing_suite<std::vector<BookmarkData>>())
    .def("get_list", &VectorAdapter<BookmarkData>::Get)
    .def("set_list", &VectorAdapter<BookmarkData>::Set)
    .def("__str__", &VectorAdapter<BookmarkData>::ToString);

  class_<std::vector<TrackData>>("TrackList")
    .def(vector_indexing_suite<std::vector<TrackData>>())
    .def("get_list", &VectorAdapter<TrackData>::Get)
    .def("set_list", &VectorAdapter<TrackData>::Set)
    .def("__str__", &VectorAdapter<TrackData>::ToString);

  class_<FileData>("FileData")
    .def_readwrite("server_id", &FileData::m_serverId)
    .def_readwrite("category", &FileData::m_categoryData)
    .def_readwrite("bookmarks", &FileData::m_bookmarksData)
    .def_readwrite("tracks", &FileData::m_tracksData)
    .def("__eq__", &FileData::operator==)
    .def("__ne__", &FileData::operator!=)
    .def("__str__", &FileDataToString);

  def("get_supported_languages", GetSupportedLanguages);
  def("get_language_index", GetLanguageIndex);
  def("timestamp_to_int", &ToSecondsSinceEpoch);
  def("point_to_latlon", &MercatorBounds::ToLatLon);

  def("export_kml", ExportKml);
  def("import_kml", ImportKml);

  def("load_classificator_types", LoadClassificatorTypes);
  def("classificator_type_to_index", ClassificatorTypeToIndex);
  def("index_to_classificator_type", IndexToClassificatorType);
}
