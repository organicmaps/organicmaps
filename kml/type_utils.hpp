#pragma once

#include "indexer/feature.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <chrono>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

namespace kml
{
using Timestamp = std::chrono::time_point<std::chrono::system_clock>;
using LocalizableString = std::unordered_map<int8_t, std::string>;
using LocalizableStringSubIndex = std::map<int8_t, uint32_t>;
using LocalizableStringIndex = std::vector<LocalizableStringSubIndex>;
using Properties = std::map<std::string, std::string>;

using MarkGroupId = uint64_t;
using MarkId = uint64_t;
using TrackId = uint64_t;
using LocalId = uint8_t;

using MarkIdCollection = std::vector<MarkId>;
using TrackIdCollection = std::vector<TrackId>;

using MarkIdSet = std::set<MarkId, std::greater<MarkId>>;
using TrackIdSet = std::set<TrackId>;

using GroupIdCollection = std::vector<MarkGroupId>;
using GroupIdSet = std::set<MarkGroupId>;

MarkGroupId constexpr kInvalidMarkGroupId = std::numeric_limits<MarkGroupId>::max();
MarkId constexpr kInvalidMarkId = std::numeric_limits<MarkId>::max();
TrackId constexpr kInvalidTrackId = std::numeric_limits<TrackId>::max();

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
  localizableStr[kDefaultLangCode] = str;
}

extern bool IsEqual(std::vector<m2::PointD> const & v1, std::vector<m2::PointD> const & v2);

struct BookmarkData;
std::string GetPreferredBookmarkName(BookmarkData const & bmData, std::string const & languageOrig);
std::string GetPreferredBookmarkStr(LocalizableString const & name, std::string const & languageNorm);
std::string GetPreferredBookmarkStr(LocalizableString const & name, feature::RegionData const & regionData,
                                    std::string const & languageNorm);
std::string GetLocalizedBookmarkType(std::vector<uint32_t> const & types, std::string const & languageOrig);

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
}  // namespace kml
