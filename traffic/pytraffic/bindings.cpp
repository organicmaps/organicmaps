#include "traffic/traffic_info.hpp"
#include "traffic/speed_groups.hpp"

#include "pyhelpers/vector_uint8.hpp"

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>

namespace
{
using namespace boost::python;

vector<uint8_t> Serialize(traffic::TrafficInfo::Coloring const & coloring)
{
  vector<uint8_t> data;
  traffic::TrafficInfo::SerializeTrafficData(coloring, data);
  return data;
}

traffic::TrafficInfo::Coloring Deserialize(vector<uint8_t> const & data)
{
  traffic::TrafficInfo::Coloring coloring;
  traffic::TrafficInfo::DeserializeTrafficData(data, coloring);
  return coloring;
}

string RoadSegmentIdRepr(traffic::TrafficInfo::RoadSegmentId const & v)
{
  stringstream ss;
  ss << "RoadSegmentId(" << v.m_fid << ", " << v.m_idx << ", " << int(v.m_dir) << ")";
  return ss.str();
}
}  // namespace

BOOST_PYTHON_MODULE(pytraffic)
{
  using namespace boost::python;

  // Register the to-python converters.
  to_python_converter<vector<uint8_t>, vector_uint8t_to_str>();
  vector_uint8t_from_python_str();

  class_<traffic::TrafficInfo::RoadSegmentId>("RoadSegmentId", init<uint32_t, uint16_t, uint8_t>())
      .def("__repr__", &RoadSegmentIdRepr)
  ;

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

  class_<traffic::TrafficInfo::Coloring>("Coloring")
      .def(map_indexing_suite<traffic::TrafficInfo::Coloring>())
  ;

  def("dumps", Serialize);
  def("loads", Deserialize);
}
