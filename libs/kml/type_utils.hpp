#pragma once

#include "geometry/point_with_altitude.hpp"

#include <chrono>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace feature
{
class RegionData;
}

namespace kml
{
using TimestampClock = std::chrono::system_clock;
using Timestamp = std::chrono::time_point<TimestampClock>;
class TimestampMillis : public Timestamp
{
public:
  TimestampMillis() = default;
  explicit TimestampMillis(Timestamp const & ts) : Timestamp{ts} {}
  TimestampMillis & operator=(Timestamp const & ts)
  {
    if (this != &ts)
      Timestamp::operator=(ts);
    return *this;
  }
};

using LocalizableString = std::unordered_map<int8_t, std::string>;
using LocalizableStringSubIndex = std::map<int8_t, uint32_t>;
using LocalizableStringIndex = std::vector<LocalizableStringSubIndex>;
using Properties = std::map<std::string, std::string>;

using MarkGroupId = uint64_t;
using MarkId = uint64_t;
using TrackId = uint64_t;
using LocalId = uint8_t;
using CompilationId = uint64_t;

using MarkIdCollection = std::vector<MarkId>;
using TrackIdCollection = std::vector<TrackId>;

using MarkIdSet = std::set<MarkId, std::greater<MarkId>>;
using TrackIdSet = std::set<TrackId>;

using GroupIdCollection = std::vector<MarkGroupId>;
using GroupIdSet = std::set<MarkGroupId>;

MarkGroupId constexpr kInvalidMarkGroupId = std::numeric_limits<MarkGroupId>::max();
MarkId constexpr kInvalidMarkId = std::numeric_limits<MarkId>::max();
MarkId constexpr kDebugMarkId = kInvalidMarkId - 1;
TrackId constexpr kInvalidTrackId = std::numeric_limits<TrackId>::max();
CompilationId constexpr kInvalidCompilationId = std::numeric_limits<CompilationId>::max();

inline uint64_t ToSecondsSinceEpoch(Timestamp const & time)
{
  auto const s = std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch());
  return static_cast<uint64_t>(s.count());
}

inline Timestamp FromSecondsSinceEpoch(uint64_t seconds)
{
  return Timestamp(std::chrono::seconds(seconds));
}

inline bool IsEqual(Timestamp const & ts1, Timestamp const & ts2)
{
  return ToSecondsSinceEpoch(ts1) == ToSecondsSinceEpoch(ts2);
}

uint32_t constexpr kEmptyStringId = 0;
double constexpr kMinLineWidth = 0.0;
double constexpr kMaxLineWidth = 100.0;
double constexpr kMinRating = 0.0;
double constexpr kMaxRating = 10.0;
uint8_t constexpr kDoubleBits = 30;

int8_t constexpr kDefaultLangCode = 0;

inline std::string GetDefaultStr(LocalizableString const & str)
{
  return (str.empty() || str.find(kDefaultLangCode) == str.end()) ? "" : str.at(kDefaultLangCode);
}

inline void SetDefaultStr(LocalizableString & localizableStr, std::string const & str)
{
  if (str.empty())
  {
    localizableStr.erase(kDefaultLangCode);
    return;
  }

  localizableStr[kDefaultLangCode] = str;
}

bool IsEqual(m2::PointD const & lhs, m2::PointD const & rhs);
bool IsEqual(geometry::PointWithAltitude const & lhs, geometry::PointWithAltitude const & rhs);

template <class T>
bool IsEqual(std::vector<T> const & lhs, std::vector<T> const & rhs)
{
  if (lhs.size() != rhs.size())
    return false;

  for (size_t i = 0; i < lhs.size(); ++i)
    if (!IsEqual(lhs[i], rhs[i]))
      return false;

  return true;
}

struct BookmarkData;
std::string GetPreferredBookmarkName(BookmarkData const & bmData, std::string_view languageOrig);
std::string GetPreferredBookmarkStr(LocalizableString const & name, std::string const & languageNorm);
std::string GetPreferredBookmarkStr(LocalizableString const & name, feature::RegionData const & regionData,
                                    std::string const & languageNorm);
std::string GetLocalizedFeatureType(std::vector<uint32_t> const & types);

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
  }

#define VISITOR_COLLECTABLE visitor(m_collectionIndex, "collectionIndex")

#define SKIP_VISITING(Type)                        \
  void operator()(Type, char const * = nullptr) {}
}  // namespace kml
