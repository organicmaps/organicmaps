#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/search_mark.hpp"

#include <memory>

namespace search_mark_lifecycle_tests
{
class RegisteredMwmInfo : public MwmInfo
{
public:
  RegisteredMwmInfo() { SetStatus(STATUS_REGISTERED); }
};

UNIT_TEST(DeactivationVisitsOnlyOneMarkAndPreservesSearchGroup)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
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
}
}  // namespace search_mark_lifecycle_tests
