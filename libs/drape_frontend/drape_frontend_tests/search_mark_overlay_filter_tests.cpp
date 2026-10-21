#include "testing/testing.hpp"

#include "drape_frontend/render_group.hpp"
#include "drape_frontend/search_mark_overlay_filter.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape/overlay_handle.hpp"
#include "drape/render_bucket.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "geometry/screenbase.hpp"

#include <memory>
#include <vector>

namespace search_mark_overlay_filter_tests
{
class RegisteredMwmInfo : public MwmInfo
{
public:
  RegisteredMwmInfo() { SetStatus(STATUS_REGISTERED); }
};

using Groups = std::vector<drape_ptr<df::RenderGroup>>;

FeatureID MakeFeature(std::shared_ptr<MwmInfo> const & mwm, uint32_t index)
{
  FeatureID id(MwmSet::MwmId(mwm), index);
  TEST(id.IsValid(), ());
  return id;
}

ScreenBase MakeScreen(double extent = 100.0)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-extent, -extent, extent, extent)));
  return screen;
}

df::RenderGroup & AddGroup(Groups & groups, gpu::Program program, df::DepthLayer layer)
{
  groups.push_back(make_unique_dp<df::RenderGroup>(df::CreateRenderState(program, layer), df::TileKey()));
  return *groups.back();
}

dp::SquareHandle * AddHandle(df::RenderGroup & group, FeatureID const & id, m2::PointD const & position,
                             m2::PointD const & offset = {}, dp::Anchor anchor = dp::Center, bool visible = true,
                             m2::PointI const & tileCoords = dp::OverlayID::NoCoordinates())
{
  auto bucket = make_unique_dp<dp::RenderBucket>(drape_ptr<dp::VertexArrayBuffer>());
  auto handle = make_unique_dp<dp::SquareHandle>(dp::OverlayID(id, kml::kInvalidMarkId, tileCoords, 0), anchor,
                                                 position, m2::PointD(24, 24), offset, 0 /* priority */,
                                                 false /* isBound */, 0 /* minVisibleScale */, true /* isBillboard */);
  auto * result = handle.get();
  result->SetIsVisible(visible);
  bucket->AddOverlayHandle(std::move(handle));
  group.AddBucket(std::move(bucket));
  return result;
}

void CheckTwoHandles(gpu::Program program)
{
  auto const mwm = std::make_shared<RegisteredMwmInfo>();
  auto const id = MakeFeature(mwm, 1);
  auto const screen = MakeScreen();
  Groups searchGroups;
  Groups regularGroups;
  AddHandle(AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer), id, {0, 0});
  searchGroups.back()->DeleteLater();  // Pending groups still have renderable buckets.
  auto & regular = AddGroup(regularGroups, program, df::DepthLayer::OverlayLayer);
  auto * near = AddHandle(regular, id, {0, 0});
  auto * far = AddHandle(regular, id, {80, 0});

  df::SearchMarkOverlayFilter filter;
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(!near->IsVisible(), ());
  TEST(far->IsVisible(), ());
  filter.RestoreHiddenSymbols();
  TEST(near->IsVisible(), ());
  TEST(far->IsVisible(), ());
}

UNIT_TEST(TwoTexturedHandlesOneFeature)
{
  CheckTwoHandles(gpu::Program::Texturing);
  CheckTwoHandles(gpu::Program::MaskedTexturing);
}

UNIT_TEST(UserMarkOverlayMatchesSpritePixelOffset)
{
  auto const screen = MakeScreen();
  df::UserMarkRenderParams params;
  params.m_pixelOffset = {0, 6};
  df::TileKey const tile(0, 0, 1);
  m2::RectD const vertices(-12, -6, 12, 18);
  auto handle = df::CreateUserMarkOverlayHandle(params, tile, vertices);
  TEST(handle, ());

  auto expected = vertices;
  expected.Offset(screen.GtoP(params.m_pivot));
  TEST_EQUAL(handle->GetPixelRect(screen, false), expected, ());
}

UNIT_TEST(SearchSpriteOffsetControlsSymbolSuppression)
{
  auto const mwm = std::make_shared<RegisteredMwmInfo>();
  auto const id = MakeFeature(mwm, 1);
  auto const screen = MakeScreen();
  df::UserMarkRenderParams params;
  params.m_featureId = id;
  params.m_pixelOffset = {0, 6};
  df::TileKey const tile(0, 0, 1);
  auto handle = df::CreateUserMarkOverlayHandle(params, tile, m2::RectD(-12, -6, 12, 18));
  handle->SetIsVisible(true);
  auto bucket = make_unique_dp<dp::RenderBucket>(drape_ptr<dp::VertexArrayBuffer>());
  bucket->AddOverlayHandle(std::move(handle));

  Groups searchGroups;
  Groups regularGroups;
  AddGroup(searchGroups, gpu::Program::BookmarkAboveText, df::DepthLayer::SearchMarkLayer).AddBucket(std::move(bucket));
  auto & regular = AddGroup(regularGroups, gpu::Program::Texturing, df::DepthLayer::OverlayLayer);
  // These 24-pixel symbols straddle the shifted sprite's bottom and top edges.
  auto * below = AddHandle(regular, id, {0, 0}, {0, 27});
  auto * above = AddHandle(regular, id, {0, 0}, {0, -21});

  df::SearchMarkOverlayFilter filter;
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(!below->IsVisible(), ());
  TEST(above->IsVisible(), ());
  filter.RestoreHiddenSymbols();
  TEST(below->IsVisible(), ());
  TEST(above->IsVisible(), ());
}

UNIT_TEST(FeatureAndGeometryMustBothMatch)
{
  auto const mwmA = std::make_shared<RegisteredMwmInfo>();
  auto const mwmB = std::make_shared<RegisteredMwmInfo>();
  auto const id = MakeFeature(mwmA, 7);
  auto const otherFeature = MakeFeature(mwmA, 8);
  auto const otherMwm = MakeFeature(mwmB, 7);
  auto const screen = MakeScreen();
  Groups searchGroups;
  Groups regularGroups;
  auto & search = AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer);
  AddHandle(search, id, {80, 0});
  AddHandle(search, otherFeature, {0, 0});
  AddHandle(search, otherMwm, {0, 0});
  auto & regular = AddGroup(regularGroups, gpu::Program::Texturing, df::DepthLayer::OverlayLayer);
  auto * handle = AddHandle(regular, id, {0, 0});

  df::SearchMarkOverlayFilter filter;
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(handle->IsVisible(), ());
  filter.RestoreHiddenSymbols();

  // The second rectangle for this ID intersects; an ID-only deduplication loses it.
  AddHandle(search, id, {0, 0});
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(!handle->IsVisible(), ());
  filter.RestoreHiddenSymbols();
  TEST(handle->IsVisible(), ());
}

UNIT_TEST(InvisibleInvalidAndTextMarksDoNotSuppress)
{
  auto const mwm = std::make_shared<RegisteredMwmInfo>();
  auto const id = MakeFeature(mwm, 3);
  auto const screen = MakeScreen();
  Groups searchGroups;
  Groups regularGroups;
  AddHandle(AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer), id, {0, 0}, {},
            dp::Center, false);
  AddHandle(AddGroup(searchGroups, gpu::Program::Text, df::DepthLayer::SearchMarkLayer), id, {0, 0});
  AddHandle(AddGroup(searchGroups, gpu::Program::TextOutlined, df::DepthLayer::SearchMarkLayer), id, {0, 0});
  AddHandle(AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer), FeatureID(), {0, 0});
  auto & regular = AddGroup(regularGroups, gpu::Program::Texturing, df::DepthLayer::OverlayLayer);
  auto * handle = AddHandle(regular, id, {0, 0});

  df::SearchMarkOverlayFilter filter;
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(handle->IsVisible(), ());
  filter.RestoreHiddenSymbols();

  AddHandle(AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer), id, {0, 0});
  handle->SetIsVisible(false);
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  filter.RestoreHiddenSymbols();
  TEST(!handle->IsVisible(), ());
}

UNIT_TEST(DeletedGroupsAreIgnored)
{
  auto const mwm = std::make_shared<RegisteredMwmInfo>();
  auto const id = MakeFeature(mwm, 9);
  auto const screen = MakeScreen();
  Groups searchGroups;
  Groups regularGroups;
  auto & deleted = AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer);
  deleted.DeleteLater();
  deleted.UpdateCanBeDeletedStatus(true, 0, nullptr);
  TEST(deleted.CanBeDeleted(), ());
  AddHandle(deleted, id, {0, 0});  // Confirm ForEachOverlay skips even a retained bucket.
  auto & regular = AddGroup(regularGroups, gpu::Program::Texturing, df::DepthLayer::OverlayLayer);
  auto * handle = AddHandle(regular, id, {0, 0});

  df::SearchMarkOverlayFilter filter;
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(handle->IsVisible(), ());
  filter.RestoreHiddenSymbols();
}

UNIT_TEST(CurrentScreenAndReplacement)
{
  auto const mwm = std::make_shared<RegisteredMwmInfo>();
  auto const id = MakeFeature(mwm, 4);
  Groups searchGroups;
  Groups regularGroups;
  AddHandle(AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer), id, {0, 0});
  auto & regular = AddGroup(regularGroups, gpu::Program::Texturing, df::DepthLayer::OverlayLayer);
  auto * handle = AddHandle(regular, id, {10, 0});
  df::SearchMarkOverlayFilter filter;

  auto screen = MakeScreen(20);
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(handle->IsVisible(), ());
  filter.RestoreHiddenSymbols();

  screen = MakeScreen(200);
  screen.SetAngle(0.2);
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(!handle->IsVisible(), ());
  filter.RestoreHiddenSymbols();

  searchGroups.clear();  // Destroy A before collecting a replacement feature.
  AddHandle(AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer), MakeFeature(mwm, 5),
            {0, 0});
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(handle->IsVisible(), ());
  filter.RestoreHiddenSymbols();

  searchGroups.clear();
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(handle->IsVisible(), ());
  filter.RestoreHiddenSymbols();
}

UNIT_TEST(ShaderRestrictionAndWrappedCopies)
{
  auto const mwm = std::make_shared<RegisteredMwmInfo>();
  auto const id = MakeFeature(mwm, 5);
  auto const screen = MakeScreen();
  Groups searchGroups;
  Groups regularGroups;
  AddHandle(AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer), id, {0, 0});
  auto & textured = AddGroup(regularGroups, gpu::Program::Texturing, df::DepthLayer::OverlayLayer);
  auto * near = AddHandle(textured, id, {0, 0}, {}, dp::Center, true, {0, 0});
  auto * wrapped = AddHandle(textured, id, {360, 0}, {}, dp::Center, true, {1, 0});
  auto & text = AddGroup(regularGroups, gpu::Program::Text, df::DepthLayer::OverlayLayer);
  auto * caption = AddHandle(text, id, {0, 0});
  auto & colored = AddGroup(regularGroups, gpu::Program::ColoredSymbol, df::DepthLayer::OverlayLayer);
  auto * coloredSymbol = AddHandle(colored, id, {0, 0});

  df::SearchMarkOverlayFilter filter;
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(textured, screen);
  TEST(!near->IsVisible(), ());
  TEST(wrapped->IsVisible(), ());
  filter.RestoreHiddenSymbols();
  filter.HideOverlappingSymbols(text, screen);
  filter.RestoreHiddenSymbols();
  filter.HideOverlappingSymbols(colored, screen);
  filter.RestoreHiddenSymbols();
  TEST(caption->IsVisible(), ());
  TEST(coloredSymbol->IsVisible(), ());
}

UNIT_TEST(OffsetsAnchorsAndPerspectiveUseCurrentRects)
{
  auto const mwm = std::make_shared<RegisteredMwmInfo>();
  auto const id = MakeFeature(mwm, 6);
  auto screen = MakeScreen();
  Groups searchGroups;
  Groups regularGroups;
  auto * search =
      AddHandle(AddGroup(searchGroups, gpu::Program::Texturing, df::DepthLayer::SearchMarkLayer), id, {0, 0}, {60, 0});
  auto & regular = AddGroup(regularGroups, gpu::Program::Texturing, df::DepthLayer::OverlayLayer);
  auto * handle = AddHandle(regular, id, {0, 0}, {60, 0}, dp::Left);
  handle->SetPivotZ(10);

  df::SearchMarkOverlayFilter filter;
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST(!handle->IsVisible(), ());
  filter.RestoreHiddenSymbols();

  screen.ApplyPerspective(0.3, 0.6, 0.7);
  auto const intersect = search->GetPixelRect(screen, true).IsIntersect(handle->GetPixelRect(screen, true));
  filter.Collect(searchGroups, screen);
  filter.HideOverlappingSymbols(regular, screen);
  TEST_EQUAL(handle->IsVisible(), !intersect, ());
  filter.RestoreHiddenSymbols();
}
}  // namespace search_mark_overlay_filter_tests
