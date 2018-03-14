#pragma once

#include "geometry/point2d.hpp"

#include <cstdint>
#include <chrono>
#include <ctime>
#include <map>
#include <sstream>
#include <vector>
#include <unordered_map>

namespace kml
{
using Timestamp = std::chrono::time_point<std::chrono::system_clock>;
using LocalizableString = std::unordered_map<int8_t, std::string>;
using LocalizableStringSubIndex = std::map<int8_t, uint32_t>;
using LocalizableStringIndex = std::vector<LocalizableStringSubIndex>;
using Id = uint64_t;
using LocalId = uint8_t;

inline uint64_t ToSecondsSinceEpoch(Timestamp const & time)
{
  auto const s = std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch());
  return static_cast<uint64_t>(s.count());
}

inline Timestamp FromSecondsSinceEpoch(uint64_t seconds)
{
  return Timestamp(std::chrono::seconds(seconds));
}

inline bool IsEqual(std::vector<m2::PointD> const & v1,
                    std::vector<m2::PointD> const & v2)
{
  if (v1.size() != v2.size())
    return false;

  double constexpr kEps = 1e-5;
  for (size_t i = 0; i < v1.size(); ++i)
  {
    if (!v1[i].EqualDxDy(v2[i], kEps))
      return false;
  }

  return true;
}

inline bool IsEqual(Timestamp const & ts1, Timestamp const & ts2)
{
  return ToSecondsSinceEpoch(ts1) == ToSecondsSinceEpoch(ts2);
}

uint32_t constexpr kEmptyStringId = 0;
double constexpr kMinLineWidth = 0.0;
double constexpr kMaxLineWidth = 100.0;

#define DECLARE_COLLECTABLE(IndexType, ...)            \
  IndexType m_collectionIndex;                         \
  template <typename Collector>                        \
  void Collect(Collector & collector)                  \
  {                                                    \
    collector.Collect(m_collectionIndex, __VA_ARGS__); \
  }                                                    \
  void ClearCollectionIndex()                          \
  {                                                    \
    m_collectionIndex.clear();                         \
  }                                                    \

#define VISITOR_COLLECTABLE visitor(m_collectionIndex, "collectionIndex")

#define SKIP_VISITING(Type) void operator()(Type, char const * = nullptr) {}

inline std::string DebugPrint(LocalizableString const & str)
{
  std::ostringstream os;
  os << "[";
  for (auto it = str.cbegin(); it != str.end();)
  {
    os << static_cast<uint32_t>(it->first) << ": " << it->second;
    ++it;
    if (it != str.end())
      os << ", ";
  }
  os << "]";
  return os.str();
}

inline std::string DebugPrint(Timestamp const & ts)
{
  auto t = std::chrono::system_clock::to_time_t(ts);
  return std::ctime(&t);
}

inline std::string DebugPrint(m2::PointD const & pt)
{
  std::ostringstream os;
  os << "[" << pt.x << ", " << pt.y << "]";
  return os.str();
}
}  // namespace kml

using kml::DebugPrint;
