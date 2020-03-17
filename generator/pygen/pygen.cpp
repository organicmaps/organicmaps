#include "generator/utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/features_vector.hpp"

#include "platform/mwm_version.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "pyhelpers/module_version.hpp"

#include <map>
#include <string>

#include "3party/boost/boost/noncopyable.hpp"
#include "3party/boost/boost/python.hpp"
#include "3party/boost/boost/shared_ptr.hpp"

using namespace feature;
namespace bp = boost::python;

namespace
{
class MwmIter;

class Mwm
{
public:
  explicit Mwm(std::string const & filename)
    : m_ds(filename)
    , m_mwmValue(m_ds.GetLocalCountryFile())
    , m_guard(std::make_unique<FeaturesLoaderGuard>(m_ds.GetDataSource(), m_ds.GetMwmId()))
  {
  }

  version::MwmVersion const & GetVersion() const { return m_mwmValue.GetMwmVersion(); }
  DataHeader::MapType GetType() const { return m_mwmValue.GetHeader().GetType(); }
  m2::RectD GetBounds() const { return m_mwmValue.GetHeader().GetBounds(); }
  size_t Size() const { return m_guard->GetNumFeatures(); }
  boost::shared_ptr<FeatureType> GetNthFeature(uint32_t index)
  {
    return m_guard->GetFeatureByIndex(index);
  }

  bp::dict GetSectionsInfo() const
  {
    bp::dict sectionsInfo;
    m_mwmValue.m_cont.ForEachInfo([&](auto const & info) { sectionsInfo[info.m_tag] = info; });
    return sectionsInfo;
  }

  MwmIter MakeMwmIter();

private:
  generator::SingleMwmDataSource m_ds;
  MwmValue m_mwmValue;
  std::unique_ptr<FeaturesLoaderGuard> m_guard;
};

class MwmIter
{
public:
  MwmIter(Mwm & mwm) : m_mwm(mwm) {}

  boost::shared_ptr<FeatureType> Next()
  {
    if (m_current == m_mwm.Size())
    {
      PyErr_SetNone(PyExc_StopIteration);
      bp::throw_error_already_set();
    }

    return m_mwm.GetNthFeature(m_current++);
  }

private:
  Mwm & m_mwm;
  uint32_t m_current = 0;
};

MwmIter Mwm::MakeMwmIter() { return MwmIter(*this); }

struct GeometryNamespace
{
};

struct MwmNamespace
{
};

struct ClassifNamespace
{
};
}  // namespace

BOOST_PYTHON_MODULE(pygen)
{
  bp::scope().attr("__version__") = PYBINDINGS_VERSION;

  classificator::Load();

  {
    bp::scope geometryNamespace = bp::class_<GeometryNamespace>("geometry");

    bp::class_<m2::PointD>("PointD", bp::init<double, double>())
        .def_readwrite("x", &m2::PointD::x)
        .def_readwrite("y", &m2::PointD::y)
        .def("__repr__", static_cast<std::string (*)(m2::PointD const &)>(m2::DebugPrint));

    bp::class_<m2::RectD>("RectD", bp::init<double, double, double, double>())
        .def(bp::init<m2::PointD, m2::PointD>())
        .add_property("min_x", &m2::RectD::minX, &m2::RectD::setMinX)
        .add_property("min_y", &m2::RectD::minY, &m2::RectD::setMinY)
        .add_property("max_x", &m2::RectD::maxX, &m2::RectD::setMaxX)
        .add_property("max_y", &m2::RectD::maxY, &m2::RectD::setMaxY)
        .add_property(
            "right_top", &m2::RectD::RightTop,
            +[](m2::RectD & self, m2::RectD const & p) {
              self.setMaxX(p.maxX());
              self.setMaxY(p.maxY());
            })
        .add_property(
            "left_bottom", &m2::RectD::LeftBottom,
            +[](m2::RectD & self, m2::RectD const & p) {
              self.setMinX(p.minX());
              self.setMinY(p.minY());
            })
        .def("__repr__", static_cast<std::string (*)(m2::RectD const &)>(m2::DebugPrint));
  }
  {
    bp::scope mwmNamespace = bp::class_<MwmNamespace>("mwm");

    bp::enum_<Metadata::EType>("EType")
        .value("FMD_CUISINE", Metadata::EType::FMD_CUISINE)
        .value("FMD_OPEN_HOURS", Metadata::EType::FMD_OPEN_HOURS)
        .value("FMD_PHONE_NUMBER", Metadata::EType::FMD_PHONE_NUMBER)
        .value("FMD_FAX_NUMBER", Metadata::EType::FMD_FAX_NUMBER)
        .value("FMD_STARS", Metadata::EType::FMD_STARS)
        .value("FMD_OPERATOR", Metadata::EType::FMD_OPERATOR)
        .value("FMD_URL", Metadata::EType::FMD_URL)
        .value("FMD_WEBSITE", Metadata::EType::FMD_WEBSITE)
        .value("FMD_INTERNET", Metadata::EType::FMD_INTERNET)
        .value("FMD_ELE", Metadata::EType::FMD_ELE)
        .value("FMD_TURN_LANES", Metadata::EType::FMD_TURN_LANES)
        .value("FMD_TURN_LANES_FORWARD", Metadata::EType::FMD_TURN_LANES_FORWARD)
        .value("FMD_TURN_LANES_BACKWARD", Metadata::EType::FMD_TURN_LANES_BACKWARD)
        .value("FMD_EMAIL", Metadata::EType::FMD_EMAIL)
        .value("FMD_POSTCODE", Metadata::EType::FMD_POSTCODE)
        .value("FMD_WIKIPEDIA", Metadata::EType::FMD_WIKIPEDIA)
        .value("FMD_FLATS", Metadata::EType::FMD_FLATS)
        .value("FMD_HEIGHT", Metadata::EType::FMD_HEIGHT)
        .value("FMD_MIN_HEIGHT", Metadata::EType::FMD_MIN_HEIGHT)
        .value("FMD_DENOMINATION", Metadata::EType::FMD_DENOMINATION)
        .value("FMD_BUILDING_LEVELS", Metadata::EType::FMD_BUILDING_LEVELS)
        .value("FMD_TEST_ID", Metadata::EType::FMD_TEST_ID)
        .value("FMD_SPONSORED_ID", Metadata::EType::FMD_SPONSORED_ID)
        .value("FMD_PRICE_RATE", Metadata::EType::FMD_PRICE_RATE)
        .value("FMD_RATING", Metadata::EType::FMD_RATING)
        .value("FMD_BANNER_URL", Metadata::EType::FMD_BANNER_URL)
        .value("FMD_LEVEL", Metadata::EType::FMD_LEVEL)
        .value("FMD_AIRPORT_IATA", Metadata::EType::FMD_AIRPORT_IATA)
        .value("FMD_BRAND", Metadata::EType::FMD_BRAND)
        .value("FMD_DURATION", Metadata::EType::FMD_DURATION);

    bp::enum_<GeomType>("GeomType")
        .value("Undefined", GeomType::Undefined)
        .value("Point", GeomType::Point)
        .value("Line", GeomType::Line)
        .value("Area", GeomType::Area);

    bp::enum_<DataHeader::MapType>("MapType")
        .value("World", DataHeader::MapType::World)
        .value("WorldCoasts", DataHeader::MapType::WorldCoasts)
        .value("Country", DataHeader::MapType::Country);

    bp::enum_<version::Format>("MwmFormat")
        .value("unknown", version::Format::unknownFormat)
        .value("v1", version::Format::v1)
        .value("v2", version::Format::v2)
        .value("v3", version::Format::v3)
        .value("v4", version::Format::v4)
        .value("v5", version::Format::v5)
        .value("v6", version::Format::v6)
        .value("v7", version::Format::v7)
        .value("v8", version::Format::v8)
        .value("v9", version::Format::v9)
        .value("last", version::Format::lastFormat);

    bp::class_<FilesContainerR::Info>("SectionInfo", bp::no_init)
        .def_readwrite("tag", &FilesContainerR::Info::m_tag)
        .def_readwrite("offset", &FilesContainerR::Info::m_offset)
        .def_readwrite("size", &FilesContainerR::Info::m_size)
        .def("__repr__", static_cast<std::string (*)(FilesContainerR::Info const &)>(DebugPrint));

    bp::class_<version::MwmVersion>("MwmVersion", bp::no_init)
        .def("format", &version::MwmVersion::GetFormat)
        .def("seconds_since_epoch", &version::MwmVersion::GetSecondsSinceEpoch)
        .def("version", &version::MwmVersion::GetVersion)
        .def("__repr__",
             static_cast<std::string (*)(version::MwmVersion const &)>(version::DebugPrint));

    bp::class_<FeatureType, boost::shared_ptr<FeatureType>, boost::noncopyable>("FeatureType",
                                                                                bp::no_init)
        .def(
            "index", +[](FeatureType const & self) { return self.GetID().m_index; })
        .def(
            "types",
            +[](FeatureType & self) {
              bp::list types;
              self.ForEachType([&](auto t) { types.append(t); });
              return types;
            })
        .def(
            "metadata",
            +[](FeatureType & self) {
              bp::dict mmetadata;
              auto const & metadata = self.GetMetadata();
              for (auto k : metadata.GetKeys())
                mmetadata[k] = metadata.Get(k);

              return mmetadata;
            })
        .def(
            "names",
            +[](FeatureType & self) {
              bp::dict mnames;
              auto const & name = self.GetNames();
              name.ForEach([&](auto code, auto && str) {
                mnames[StringUtf8Multilang::GetLangByCode(code)] = std::move(str);
              });

              return mnames;
            })
        .def(
            "readable_name",
            +[](FeatureType & self) {
              std::string name;
              self.GetReadableName(name);
              return name;
            })
        .def("rank", &FeatureType::GetRank)
        .def("population", &FeatureType::GetPopulation)
        .def("road_number", &FeatureType::GetRoadNumber)
        .def("house_number", &FeatureType::GetHouseNumber)
        .def("postcode", &FeatureType::GetPostcode, bp::return_internal_reference<>())
        .def("layer", &FeatureType::GetLayer)
        .def("geom_type", &FeatureType::GetGeomType)
        .def("center", &FeatureType::GetCenter)
        .def(
            "geometry",
            +[](FeatureType & self) {
              bp::list points;
              switch (self.GetGeomType())
              {
              case GeomType::Point:
              case GeomType::Line:
              {
                self.ForEachPoint([&](auto const & p) { points.append(p); },
                                  FeatureType::BEST_GEOMETRY);
              }
              break;
              case GeomType::Area:
              {
                for (auto const & p : self.GetTrianglesAsPoints(FeatureType::BEST_GEOMETRY))
                  points.append(p);
              }
              break;
              case GeomType::Undefined: break;
              default: UNREACHABLE();
              }
              return points;
            })
        .def(
            "limit_rect",
            +[](FeatureType & self) { return self.GetLimitRect(FeatureType::BEST_GEOMETRY); })
        .def(
            "__repr__",
            +[](FeatureType & self) { return self.DebugString(FeatureType::BEST_GEOMETRY); });

    bp::class_<MwmIter>("MwmIter", bp::no_init)
        .def(
            "__iter__", +[](MwmIter self) { return self; })
        .def("__next__", &MwmIter::Next)
        .def("next", &MwmIter::Next);

    bp::class_<Mwm, boost::noncopyable>("Mwm", bp::init<std::string>())
        .def("version", &Mwm::GetVersion, bp::return_internal_reference<>())
        .def("type", &Mwm::GetType)
        .def("bounds", &Mwm::GetBounds)
        .def("sections_info", &Mwm::GetSectionsInfo)
        .def("__iter__", &Mwm::MakeMwmIter)
        .def("__len__", &Mwm::Size);
  }
  {
    bp::scope classifNamespace = bp::class_<ClassifNamespace>("classif");

    bp::def(
        "readable_type", +[](uint32_t t) { return classif().GetReadableObjectName(t); });
  }
}
