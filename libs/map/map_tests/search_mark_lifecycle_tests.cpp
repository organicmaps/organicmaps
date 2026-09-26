#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/search_mark.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include <memory>

namespace search_mark_lifecycle_tests
{
class RegisteredMwmInfo : public MwmInfo
{
public:
  RegisteredMwmInfo()
  {
    SetStatus(STATUS_REGISTERED);
    m_file = platform::LocalCountryFile({}, platform::CountryFile("Search marks"), 0);
  }
};

UNIT_TEST(DeactivationVisitsOnlyOneMarkAndPreservesSearchGroup)
{
  Framework framework({}, false /* loadMaps */);
  auto & bookmarks = framework.GetBookmarkManager();
  SearchMarks marks;
  marks.SetBookmarkManager(&bookmarks);

  auto const mwm = std::make_shared<RegisteredMwmInfo>();
  FeatureID const first(MwmSet::MwmId(mwm), 1);
  FeatureID const second(MwmSet::MwmId(mwm), 2);
  TEST(first.IsValid(), ());
  TEST(second.IsValid(), ());

  kml::MarkId firstMarkId;
  kml::MarkId secondMarkId;
  {
    auto session = bookmarks.GetEditSession();
    auto * firstMark = session.CreateUserMark<SearchMarkPoint>({0, 0});
    firstMark->SetFoundFeature(first);
    firstMarkId = firstMark->GetId();
    auto * secondMark = session.CreateUserMark<SearchMarkPoint>({1, 1});
    secondMark->SetFoundFeature(second);
    secondMarkId = secondMark->GetId();
  }

  auto const before = bookmarks.GetUserMarkIds(UserMark::Type::SEARCH).size();
  TEST_EQUAL(before, 2, ());
  auto const * firstMark = bookmarks.GetMark<SearchMarkPoint>(firstMarkId);
  auto const * secondMark = bookmarks.GetMark<SearchMarkPoint>(secondMarkId);
  auto const originalOpacity = firstMark->GetSymbolOpacity();
  TEST_EQUAL(originalOpacity, secondMark->GetSymbolOpacity(), ());

  marks.OnDeactivate(first);
  TEST(marks.IsVisited(first), ());
  TEST(!marks.IsVisited(second), ());
  auto const visitedOpacity = firstMark->GetSymbolOpacity();
  TEST_LESS(visitedOpacity, originalOpacity, ());
  TEST_EQUAL(secondMark->GetSymbolOpacity(), originalOpacity, ());
  TEST_EQUAL(bookmarks.GetUserMarkIds(UserMark::Type::SEARCH).size(), before, ());

  marks.OnDeactivate(first);
  TEST_EQUAL(firstMark->GetSymbolOpacity(), visitedOpacity, ());
  TEST_EQUAL(secondMark->GetSymbolOpacity(), originalOpacity, ());
  TEST_EQUAL(bookmarks.GetUserMarkIds(UserMark::Type::SEARCH).size(), before, ());

  FeatureID const absent(MwmSet::MwmId(mwm), 3);
  marks.OnDeactivate(absent);
  TEST(!marks.IsVisited(absent), ());
  TEST_EQUAL(bookmarks.GetUserMarkIds(UserMark::Type::SEARCH).size(), before, ());
}

class Hotel : public osm::MapObject
{
public:
  explicit Hotel(FeatureID const & id)
  {
    m_featureID = id;
    m_mercator = {0, 0};
    m_geomType = feature::GeomType::Point;
    m_types.Add(classif().GetTypeByPath({"tourism", "hotel"}));
    m_name.Add("default", "Test hotel");
  }
};

void CheckHotelDeparture(bool closePlacePage)
{
  Framework framework({}, false /* loadMaps */);
  auto & bookmarks = framework.GetBookmarkManager();
  auto const mwm = std::make_shared<RegisteredMwmInfo>();
  FeatureID const hotelId(MwmSet::MwmId(mwm), 1);
  FeatureID const otherId(MwmSet::MwmId(mwm), 2);
  kml::MarkId hotelMarkId;
  kml::MarkId otherMarkId;
  {
    auto session = bookmarks.GetEditSession();
    auto * hotel = session.CreateUserMark<SearchMarkPoint>({0, 0});
    hotel->SetFoundFeature(hotelId);
    hotelMarkId = hotel->GetId();
    auto * other = session.CreateUserMark<SearchMarkPoint>({1, 1});
    other->SetFoundFeature(otherId);
    otherMarkId = other->GetId();
  }

  search::Result first(m2::PointD(0, 0), "Hotel");
  first.SetType(search::Result::Type::LatLon);
  framework.SelectSearchResult(first, false /* animation */);
  // Supply hotel metadata without loading a map; selection and deactivation use Framework's public API.
  auto hotelFeature = FeatureType::CreateFromMapObject(Hotel(hotelId));
  framework.GetCurrentPlacePageInfo().SetFromFeatureType(*hotelFeature);
  TEST(framework.GetCurrentPlacePageInfo().IsHotel(), ());
  TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetID(), hotelId, ());
  auto const opacity = bookmarks.GetMark<SearchMarkPoint>(hotelMarkId)->GetSymbolOpacity();

  if (closePlacePage)
  {
    framework.DeactivateMapSelection();
    TEST(!framework.HasPlacePageInfo(), ());
  }
  else
  {
    search::Result second(m2::PointD(1, 1), "Next result");
    second.SetType(search::Result::Type::LatLon);
    framework.SelectSearchResult(second, false /* animation */);
    TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetMercator(), second.GetFeatureCenter(), ());
  }

  auto const visitedOpacity = bookmarks.GetMark<SearchMarkPoint>(hotelMarkId)->GetSymbolOpacity();
  TEST_LESS(visitedOpacity, opacity, ());
  TEST_EQUAL(bookmarks.GetMark<SearchMarkPoint>(otherMarkId)->GetSymbolOpacity(), opacity, ());
  TEST_EQUAL(bookmarks.GetUserMarkIds(UserMark::Type::SEARCH).size(), 2, ());

  // Result refreshes must retain the Framework-owned visited state.
  first.FromFeature(hotelId, classif().GetTypeByPath({"tourism", "hotel"}), 0, {});
  search::Results results;
  results.AddResultNoChecks(std::move(first));
  framework.FillSearchResultsMarks(true /* clear */, results);
  auto const & refreshed = bookmarks.GetUserMarkIds(UserMark::Type::SEARCH);
  TEST_EQUAL(refreshed.size(), 1, ());
  TEST_EQUAL(bookmarks.GetMark<SearchMarkPoint>(*refreshed.begin())->GetSymbolOpacity(), visitedOpacity, ());

  framework.GetSearchAPI().CancelAllSearches();
  TEST(bookmarks.GetUserMarkIds(UserMark::Type::SEARCH).empty(), ());
}

UNIT_TEST(ClosingHotelVisitsMarkAndPreservesResults)
{
  CheckHotelDeparture(true /* closePlacePage */);
}

UNIT_TEST(SelectingNextResultVisitsHotelAndPreservesResults)
{
  CheckHotelDeparture(false /* closePlacePage */);
}
}  // namespace search_mark_lifecycle_tests
