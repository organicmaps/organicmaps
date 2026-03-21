#include "testing/testing.hpp"

#include "coding/serdes_json.hpp"
#include "coding/writer.hpp"

#include "base/string_utils.hpp"
#include "base/visitor.hpp"

#include <array>
#include <chrono>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

namespace
{
template <typename Ptr>
bool SamePtrValue(Ptr const & lhs, Ptr const & rhs)
{
  return (!lhs && !rhs) || (lhs && rhs && *lhs == *rhs);
}

template <typename T>
bool TestSerDes(T const & value)
{
  std::string jsonStr;
  {
    using Sink = MemWriter<std::string>;
    Sink sink(jsonStr);
    coding::SerializerJson<Sink> ser(sink);
    ser(value);
  }

  T deserializedValue;
  try
  {
    coding::DeserializerJson des(jsonStr);
    des(deserializedValue);
  }
  catch (base::Json::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while parsing json string, reason:", exception.what(), "json:", jsonStr));
    return false;
  }
  return deserializedValue == value;
}

enum class TestEnum
{
  Value0 = 0,
  Value1,
  Value2,
  Count
};

struct ValueTypes
{
  DECLARE_VISITOR(visitor(m_boolValue, "boolValue"), visitor(m_uint8Value, "uint8Value"),
                  visitor(m_uint32Value, "uint32Value"), visitor(m_uint64Value, "uint64Value"),
                  visitor(m_int8Value, "int8Value"), visitor(m_int32Value, "int32Value"),
                  visitor(m_int64Value, "int64Value"), visitor(m_doubleValue, "doubleValue"),
                  visitor(m_stringValue, "stringValue"), visitor(m_enumValue, "enumValue"),
                  visitor(m_timePointValue, "timePointValue"))

  ValueTypes() = default;
  ValueTypes(uint32_t testCounter)
    : m_boolValue(static_cast<bool>(testCounter % 2))
    , m_uint8Value(std::numeric_limits<uint8_t>::max() - static_cast<uint8_t>(testCounter))
    , m_uint32Value(std::numeric_limits<uint32_t>::max() - testCounter)
    , m_uint64Value(std::numeric_limits<uint64_t>::max() - testCounter)
    , m_int8Value(std::numeric_limits<int8_t>::min() + static_cast<int8_t>(testCounter))
    , m_int32Value(std::numeric_limits<int32_t>::min() + static_cast<int32_t>(testCounter))
    , m_int64Value(std::numeric_limits<int64_t>::min() + static_cast<int64_t>(testCounter))
    , m_doubleValue(std::numeric_limits<double>::max() - testCounter)
    , m_stringValue(strings::to_string(testCounter))
    , m_enumValue(static_cast<TestEnum>(testCounter % static_cast<uint32_t>(TestEnum::Count)))
    , m_timePointValue(std::chrono::system_clock::now())
  {}

  bool operator==(ValueTypes const & rhs) const
  {
    return m_boolValue == rhs.m_boolValue && m_uint8Value == rhs.m_uint8Value && m_uint32Value == rhs.m_uint32Value &&
           m_uint64Value == rhs.m_uint64Value && m_int8Value == rhs.m_int8Value && m_int32Value == rhs.m_int32Value &&
           m_int64Value == rhs.m_int64Value && m_doubleValue == rhs.m_doubleValue &&
           m_stringValue == rhs.m_stringValue && m_enumValue == rhs.m_enumValue &&
           m_timePointValue == rhs.m_timePointValue;
  }

  bool m_boolValue;
  uint8_t m_uint8Value;
  uint32_t m_uint32Value;
  uint64_t m_uint64Value;
  int8_t m_int8Value;
  int32_t m_int32Value;
  int64_t m_int64Value;
  double m_doubleValue;
  std::string m_stringValue;
  TestEnum m_enumValue;
  std::chrono::system_clock::time_point m_timePointValue;
};

struct ObjectTypes
{
  DECLARE_VISITOR(visitor(m_pointValue, "pointValue"), visitor(m_latLonValue, "latLonValue"),
                  visitor(m_pairValue, "pairValue"))

  ObjectTypes() = default;
  ObjectTypes(uint32_t testCounter)
    : m_pointValue(testCounter, testCounter)
    , m_latLonValue(testCounter, testCounter)
    , m_pairValue(testCounter, strings::to_string(testCounter))
  {}

  bool operator==(ObjectTypes const & rhs) const
  {
    return m_pointValue == rhs.m_pointValue && m_latLonValue == rhs.m_latLonValue && m_pairValue == rhs.m_pairValue;
  }

  m2::PointD m_pointValue;
  ms::LatLon m_latLonValue;
  std::pair<uint32_t, std::string> m_pairValue;
};

struct PointerTypes
{
  DECLARE_VISITOR(visitor(m_uniquePtrValue, "uniquePtrValue"), visitor(m_sharedPtrValue, "sharedPtrValue"))

  PointerTypes() = default;
  PointerTypes(uint32_t testCounter)
  {
    m_uniquePtrValue = std::make_unique<ValueTypes>(testCounter);
    m_sharedPtrValue = std::make_shared<ValueTypes>(testCounter);
  }

  bool operator==(PointerTypes const & rhs) const
  {
    return SamePtrValue(m_uniquePtrValue, rhs.m_uniquePtrValue) && SamePtrValue(m_sharedPtrValue, rhs.m_sharedPtrValue);
  }

  std::unique_ptr<ValueTypes> m_uniquePtrValue;
  std::shared_ptr<ValueTypes> m_sharedPtrValue;
};

struct ArrayTypes
{
  DECLARE_VISITOR(visitor(m_arrayValue, "arrayValue"), visitor(m_dequeValue, "dequeValue"),
                  visitor(m_vectorValue, "vectorValue"), visitor(m_mapValue, "mapValue"),
                  visitor(m_unorderedSetValue, "unorderedSetValue"))

  ArrayTypes() = default;
  ArrayTypes(uint32_t testCounter)
    : m_arrayValue({{testCounter, testCounter + 1, testCounter + 2}})
    , m_dequeValue({testCounter + 2, testCounter + 1, testCounter})
    , m_vectorValue({testCounter, testCounter + 2, testCounter + 1})
    , m_mapValue({{testCounter, testCounter}, {testCounter + 1, testCounter + 1}})
    , m_unorderedSetValue({testCounter + 2, testCounter, testCounter + 1})
  {}

  bool operator==(ArrayTypes const & rhs) const
  {
    return m_arrayValue == rhs.m_arrayValue && m_dequeValue == rhs.m_dequeValue && m_vectorValue == rhs.m_vectorValue &&
           m_mapValue == rhs.m_mapValue && m_unorderedSetValue == rhs.m_unorderedSetValue;
  }

  std::array<uint32_t, 3> m_arrayValue;
  std::deque<uint32_t> m_dequeValue;
  std::vector<uint32_t> m_vectorValue;
  std::map<uint32_t, uint32_t> m_mapValue;
  std::unordered_set<uint32_t> m_unorderedSetValue;
};
}  // namespace

UNIT_TEST(SerdesJsonTest)
{
  {
    ValueTypes valueTypes(0);
    TEST(TestSerDes(valueTypes), ());

    ObjectTypes objectTypes(0);
    TEST(TestSerDes(objectTypes), ());

    PointerTypes pointersTypes(0);
    TEST(TestSerDes(pointersTypes), ());

    ArrayTypes arrayTypes(0);
    TEST(TestSerDes(arrayTypes), ());
  }

  {
    std::pair<std::string, m2::PointD> testValue = {"test", m2::PointD(1.0, 2.0)};
    TEST(TestSerDes(testValue), ());
  }

  {
    std::pair<m2::PointD, m2::PointD> testValue = {m2::PointD(1.0, 2.0), m2::PointD(2.0, 3.0)};
    TEST(TestSerDes(testValue), ());
  }

  {
    std::pair<std::string, std::pair<std::string, std::string>> testValue = {"test", {"test1", "test2"}};
    TEST(TestSerDes(testValue), ());
  }

  {
    std::pair<std::string, ValueTypes> testValue = {"test", ValueTypes(0)};
    TEST(TestSerDes(testValue), ());
  }

  {
    std::array<ObjectTypes, 2> testValue = {{ObjectTypes(0), ObjectTypes(1)}};
    TEST(TestSerDes(testValue), ());
  }

  {
    struct Hasher
    {
      size_t operator()(std::pair<std::string, std::string> const & item) const
      {
        return m_hasher(item.first + item.second);
      }

      std::hash<std::string> m_hasher;
    };

    std::unordered_set<std::pair<std::string, std::string>, Hasher> testValue = {
        {"ab", "ab"}, {"ef", "ef"}, {"cd", "cd"}};
    TEST(TestSerDes(testValue), ());
  }

  {
    std::vector<std::vector<uint32_t>> testValue;
    for (uint32_t i = 0; i < 5; ++i)
      testValue.push_back({i, i, i});
    TEST(TestSerDes(testValue), ());
  }

  {
    std::vector<ValueTypes> valuesVector;
    for (uint32_t i = 0; i < 5; ++i)
      valuesVector.push_back(ValueTypes(i));
    TEST(TestSerDes(valuesVector), ());
  }

  {
    std::map<uint32_t, ValueTypes> valuesMap;
    for (uint32_t i = 0; i < 5; ++i)
      valuesMap.insert(std::make_pair(i, ValueTypes(i)));
    TEST(TestSerDes(valuesMap), ());
  }

  {
    std::vector<ObjectTypes> objectsVector;
    for (uint32_t i = 0; i < 5; ++i)
      objectsVector.push_back(ObjectTypes(i));
    TEST(TestSerDes(objectsVector), ());
  }

  {
    std::map<uint32_t, ObjectTypes> objectsMap;
    for (uint32_t i = 0; i < 5; ++i)
      objectsMap.insert(std::make_pair(i, ObjectTypes(i)));
    TEST(TestSerDes(objectsMap), ());
  }
}
