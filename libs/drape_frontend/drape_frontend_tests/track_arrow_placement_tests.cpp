#include "testing/testing.hpp"

#include "drape_frontend/user_mark_shapes.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/overlay_handle.hpp"

#include "geometry/screenbase.hpp"

#include <vector>

UNIT_TEST(TrackDirectionArrowPanDoesNotPropagateCollisions)
{
  df::VisualParams::Init(1.0, 512);
  ScreenBase screen;
  screen.OnSize(0, 0, 120, 100);
  screen.SetScale(0.001);
  std::vector<drape_ptr<dp::SquareHandle>> handles;
  df::TrackArrowPlacement arrows;
  // Each candidate is within 48 dp of the previous one. The highest priority arrow
  // leaves the placement area on pan, while the arrow at x=0 remains on screen.
  for (int x : {-120, -80, -40, 0})
  {
    handles.push_back(make_unique_dp<dp::SquareHandle>(dp::OverlayID{}, dp::Center,
                                                       m2::PointD(x * screen.GetScale(), 0), m2::PointD(4, 4),
                                                       m2::PointD::Zero(), 1 - x, false, 1, false));
  }
  auto const pan = [&](int x)
  {
    screen.SetOrg({x * screen.GetScale(), 0});
    for (auto const & handle : handles)
      arrows.Add(make_ref(handle));
    arrows.Place(screen);
  };
  pan(-20);
  TEST(handles.front()->IsVisible(), ());
  TEST(!handles[2]->IsVisible(), ());
  TEST(!handles.back()->IsVisible(), ());
  pan(20);
  TEST(!handles.front()->IsVisible(), ());
  TEST(handles[1]->IsVisible(), ());
  TEST(!handles.back()->IsVisible(), ("The neighbor at x=-40 still suppresses this arrow"));
}
