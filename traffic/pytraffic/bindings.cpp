#include "traffic/traffic_info.hpp"
#include "traffic/speed_groups.hpp"

#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"

#include "std/cstdint.hpp"
#include "std/map.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

#include "pyhelpers/vector_list_conversion.hpp"
#include "pyhelpers/vector_uint8.hpp"

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace
{
using namespace boost::python;

struct SegmentSpeeds
{
  SegmentSpeeds() = default;
  SegmentSpeeds(double weightedSpeed, double weightedRefSpeed, double weight)
    : m_weightedSpeed(weightedSpeed), m_weightedRefSpeed(weightedRefSpeed), m_weight(weight)
  {
  }

  double m_weightedSpeed = 0;
  double m_weightedRefSpeed = 0;
  double m_weight = 0;
};

using SegmentMapping = map<traffic::TrafficInfo::RoadSegmentId, SegmentSpeeds>;

string SegmentSpeedsRepr(SegmentSpeeds const & v)
{
  ostringstream ss;
  ss << "SegmentSpeeds("
     << " weighted_speed=" << v.m_weightedSpeed << " weighted_ref_speed=" << v.m_weightedRefSpeed
     << " weight=" << v.m_weight << " )";
  return ss.str();
}

traffic::TrafficInfo::Coloring TransformToSpeedGroups(SegmentMapping const & segmentMapping)
{
  double const kEps = 1e-9;
  traffic::TrafficInfo::Coloring result;
  for (auto const & kv : segmentMapping)
  {
    double const ws = kv.second.m_weightedSpeed;
    double const wrs = kv.second.m_weightedRefSpeed;
    double const w = kv.second.m_weight;
    if (my::AlmostEqualAbs(w, 0.0, kEps))
    {
      LOG(LWARNING, ("A traffic segment has zero weight."));
      continue;
    }
    double const u = ws / w;
    double const v = wrs / w;
    bool const uz = my::AlmostEqualAbs(u, 0.0, kEps);
    bool const vz = my::AlmostEqualAbs(v, 0.0, kEps);
    if (uz && vz)
    {
      result[kv.first] = traffic::SpeedGroup::TempBlock;
    }
    else if (vz)
    {
      LOG(LWARNING, ("A traffic segment has zero reference speed."));
      continue;
    }
    else
    {
      double p = 100.0 * u / v;
      p = my::clamp(p, 0.0, 100.0);
      result[kv.first] = traffic::GetSpeedGroupByPercentage(p);
    }
  }
  return result;
}

string RoadSegmentIdRepr(traffic::TrafficInfo::RoadSegmentId const & v)
{
  ostringstream ss;
  ss << "RoadSegmentId(" << v.m_fid << ", " << v.m_idx << ", " << int(v.m_dir) << ")";
  return ss.str();
}

boost::python::list GenerateTrafficKeys(string const & mwmPath)
{
  vector<traffic::TrafficInfo::RoadSegmentId> result;
  traffic::TrafficInfo::ExtractTrafficKeys(mwmPath, result);
  return std_vector_to_python_list(result);
}

vector<uint8_t> GenerateTrafficValues(vector<traffic::TrafficInfo::RoadSegmentId> const & keys,
                                      boost::python::dict const & segmentMappingDict)
{
  SegmentMapping segmentMapping;
  boost::python::list mappingKeys = segmentMappingDict.keys();
  for (size_t i = 0; i < len(mappingKeys); ++i)
  {
    object curArg = segmentMappingDict[mappingKeys[i]];
    if (curArg)
      segmentMapping[extract<traffic::TrafficInfo::RoadSegmentId>(mappingKeys[i])] =
          extract<SegmentSpeeds>(segmentMappingDict[mappingKeys[i]]);
  }

  traffic::TrafficInfo::Coloring const knownColors = TransformToSpeedGroups(segmentMapping);
  traffic::TrafficInfo::Coloring coloring;
  traffic::TrafficInfo::CombineColorings(keys, knownColors, coloring);

  vector<traffic::SpeedGroup> values(coloring.size());

  size_t i = 0;
  for (auto const & kv : coloring)
  {
    ASSERT_EQUAL(kv.first, keys[i], ());
    values[i] = kv.second;
    ++i;
  }
  ASSERT_EQUAL(i, values.size(), ());

  vector<uint8_t> buf;
  traffic::TrafficInfo::SerializeTrafficValues(values, buf);
  return buf;
}

vector<uint8_t> GenerateTrafficValuesFromList(boost::python::list const & keys,
                                              boost::python::dict const & segmentMappingDict)
{
  vector<traffic::TrafficInfo::RoadSegmentId> keysVec =
      python_list_to_std_vector<traffic::TrafficInfo::RoadSegmentId>(keys);

  return GenerateTrafficValues(keysVec, segmentMappingDict);
}

vector<uint8_t> GenerateTrafficValuesFromBinary(vector<uint8_t> const & keysBlob,
                                                boost::python::dict const & segmentMappingDict)
{
  vector<traffic::TrafficInfo::RoadSegmentId> keys;
  traffic::TrafficInfo::DeserializeTrafficKeys(keysBlob, keys);

  return GenerateTrafficValues(keys, segmentMappingDict);
}

void LoadClassificator(string const & classifPath)
{
  GetPlatform().SetResourceDir(classifPath);
  classificator::Load();
}
}  // namespace

BOOST_PYTHON_MODULE(pytraffic)
{
  using namespace boost::python;

  // Register the to-python converters.
  to_python_converter<vector<uint8_t>, vector_uint8t_to_str>();
  vector_uint8t_from_python_str();

  class_<SegmentSpeeds>("SegmentSpeeds", init<double, double, double>())
      .def("__repr__", &SegmentSpeedsRepr)
      .def_readwrite("weighted_speed", &SegmentSpeeds::m_weightedSpeed)
      .def_readwrite("weighted_ref_speed", &SegmentSpeeds::m_weightedRefSpeed)
      .def_readwrite("weight", &SegmentSpeeds::m_weight)
  ;

  class_<traffic::TrafficInfo::RoadSegmentId>("RoadSegmentId", init<uint32_t, uint16_t, uint8_t>())
      .def("__repr__", &RoadSegmentIdRepr)
      .add_property("fid", &traffic::TrafficInfo::RoadSegmentId::GetFid)
      .add_property("idx", &traffic::TrafficInfo::RoadSegmentId::GetIdx)
      .add_property("dir", &traffic::TrafficInfo::RoadSegmentId::GetDir)
  ;

  class_<std::vector<traffic::TrafficInfo::RoadSegmentId>>("RoadSegmentIdVec")
      .def(vector_indexing_suite<std::vector<traffic::TrafficInfo::RoadSegmentId>>());

  enum_<traffic::SpeedGroup>("SpeedGroup")
      .value("G0", traffic::SpeedGroup::G0)
      .value("G1", traffic::SpeedGroup::G1)
      .value("G2", traffic::SpeedGroup::G2)
      .value("G3", traffic::SpeedGroup::G3)
      .value("G4", traffic::SpeedGroup::G4)
      .value("G5", traffic::SpeedGroup::G5)
      .value("TempBlock", traffic::SpeedGroup::TempBlock)
      .value("Unknown", traffic::SpeedGroup::Unknown)
  ;

  def("load_classificator", LoadClassificator);
  def("generate_traffic_keys", GenerateTrafficKeys);
  def("generate_traffic_values_from_list", GenerateTrafficValuesFromList);
  def("generate_traffic_values_from_binary", GenerateTrafficValuesFromBinary);
}
