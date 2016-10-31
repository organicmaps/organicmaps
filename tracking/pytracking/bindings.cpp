#include "tracking/protocol.hpp"

#include "coding/traffic.hpp"

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace
{
using namespace boost::python;

// Converts a std::pair instance to a Python tuple.
template <typename T1, typename T2>
struct pair_to_tuple
{
  static PyObject * convert(pair<T1, T2> const & p)
  {
    return incref(make_tuple(p.first, p.second).ptr());
  }
  
  static PyTypeObject const * get_pytype() { return &PyTuple_Type; }
};

template <typename T1, typename T2>
struct pair_to_python_converter
{
  pair_to_python_converter() { to_python_converter<pair<T1, T2>, pair_to_tuple<T1, T2>, true>(); }
};

// Converts a vector<uint8_t> to/from Python str.
struct vector_uint8t_to_str
{
  static PyObject * convert(vector<uint8_t> const & v)
  {
    str s(reinterpret_cast<char const *>(v.data()), v.size());
    return incref(s.ptr());
  }
};

struct vector_uint8t_from_python_str
{
  vector_uint8t_from_python_str()
  {
    converter::registry::push_back(&convertible, &construct, type_id<vector<uint8_t>>());
  }

  static void * convertible(PyObject * obj_ptr)
  {
    if (!PyString_Check(obj_ptr))
      return nullptr;
    return obj_ptr;
  }

  static void construct(PyObject * obj_ptr, converter::rvalue_from_python_stage1_data * data)
  {
    const char * value = PyString_AsString(obj_ptr);
    if (value == nullptr)
      throw_error_already_set();
    void * storage =
        ((converter::rvalue_from_python_storage<vector<uint8_t>> *)data)->storage.bytes;
    new (storage) vector<uint8_t>(value, value + PyString_Size(obj_ptr));
    data->convertible = storage;
  }
};
}  // namespace

BOOST_PYTHON_MODULE(pytracking)
{
  using namespace boost::python;
  using tracking::Protocol;

  // Register the to-python converters.
  pair_to_python_converter<Protocol::PacketType, size_t>();
  to_python_converter<vector<uint8_t>, vector_uint8t_to_str>();
  vector_uint8t_from_python_str();

  class_<Protocol::DataElementsVec>("DataElementsVec")
      .def(vector_indexing_suite<Protocol::DataElementsVec>());

  class_<ms::LatLon>("LatLon")
      .def_readwrite("lat", &ms::LatLon::lat)
      .def_readwrite("lon", &ms::LatLon::lon);

  class_<coding::TrafficGPSEncoder::DataPoint>("DataPoint")
      .def(init<uint64_t, ms::LatLon const &>())
      .def_readwrite("timestamp", &coding::TrafficGPSEncoder::DataPoint::m_timestamp)
      .def_readwrite("coords", &coding::TrafficGPSEncoder::DataPoint::m_latLon);

  enum_<Protocol::PacketType>("PacketType")
      .value("AuthV0", Protocol::PacketType::AuthV0)
      .value("DataV0", Protocol::PacketType::DataV0)
      .value("CurrentAuth", Protocol::PacketType::CurrentAuth)
      .value("CurrentData", Protocol::PacketType::CurrentData);

  vector<uint8_t> (*CreateDataPacket1)(Protocol::DataElementsCirc const &) =
      &Protocol::CreateDataPacket;
  vector<uint8_t> (*CreateDataPacket2)(Protocol::DataElementsVec const &) =
      &Protocol::CreateDataPacket;

  class_<Protocol>("Protocol")
      .def("CreateAuthPacket", &Protocol::CreateAuthPacket)
      .staticmethod("CreateAuthPacket")
      .def("CreateDataPacket", CreateDataPacket1)
      .def("CreateDataPacket", CreateDataPacket2)
      .staticmethod("CreateDataPacket")
      .def("CreateHeader", &Protocol::CreateHeader)
      .staticmethod("CreateHeader")
      .def("DecodeHeader", &Protocol::DecodeHeader)
      .staticmethod("DecodeHeader")
      .def("DecodeDataPacket", &Protocol::DecodeDataPacket)
      .staticmethod("DecodeDataPacket");
}
