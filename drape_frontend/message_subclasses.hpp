#pragma once

#include "drape_frontend/circles_pack_shape.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/drape_api.hpp"
#include "drape_frontend/drape_api_builder.hpp"
#include "drape_frontend/gps_track_point.hpp"
#include "drape_frontend/gui/layer_render.hpp"
#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/message.hpp"
#include "drape_frontend/my_position.hpp"
#include "drape_frontend/overlay_batcher.hpp"
#include "drape_frontend/postprocess_renderer.hpp"
#include "drape_frontend/render_state.hpp"
#include "drape_frontend/route_builder.hpp"
#include "drape_frontend/selection_shape.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/traffic_generator.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/user_mark_shapes.hpp"
#include "drape_frontend/user_marks_provider.hpp"

#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"
#include "drape/viewport.hpp"

#include "geometry/polyline2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"
#include "geometry/triangle2d.hpp"

#include "platform/location.hpp"

#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/set.hpp"
#include "std/shared_ptr.hpp"
#include "std/utility.hpp"

namespace df
{
class BaseBlockingMessage : public Message
{
public:
  struct Blocker
  {
    void Wait()
    {
      unique_lock<mutex> lock(m_lock);
      m_signal.wait(lock, [this]{return !m_blocked;} );
    }

  private:
    friend class BaseBlockingMessage;

    void Signal()
    {
      lock_guard<mutex> lock(m_lock);
      m_blocked = false;
      m_signal.notify_one();
    }

  private:
    mutex m_lock;
    condition_variable m_signal;
    bool m_blocked = true;
  };

  BaseBlockingMessage(Blocker & blocker)
    : m_blocker(blocker)
  {
  }

  ~BaseBlockingMessage()
  {
    m_blocker.Signal();
  }

private:
  Blocker & m_blocker;
};

class BaseTileMessage : public Message
{
public:
  BaseTileMessage(TileKey const & key)
    : m_tileKey(key) {}

  TileKey const & GetKey() const { return m_tileKey; }

private:
  TileKey m_tileKey;
};

class FinishReadingMessage : public Message
{
public:
  FinishReadingMessage() = default;

  Type GetType() const override { return Message::FinishReading; }
};

class FinishTileReadMessage : public Message
{
public:
  template<typename T> FinishTileReadMessage(T && tiles, bool forceUpdateUserMarks)
    : m_tiles(forward<T>(tiles))
    , m_forceUpdateUserMarks(forceUpdateUserMarks)
  {}

  Type GetType() const override { return Message::FinishTileRead; }

  TTilesCollection const & GetTiles() const { return m_tiles; }
  TTilesCollection && MoveTiles() { return move(m_tiles); }
  bool NeedForceUpdateUserMarks() const { return m_forceUpdateUserMarks; }

private:
  TTilesCollection m_tiles;
  bool m_forceUpdateUserMarks;
};

class FlushRenderBucketMessage : public BaseTileMessage
{
public:
  FlushRenderBucketMessage(TileKey const & key, dp::GLState const & state,
                           drape_ptr<dp::RenderBucket> && buffer)
    : BaseTileMessage(key)
    , m_state(state)
    , m_buffer(move(buffer))
  {}

  Type GetType() const override { return Message::FlushTile; }
  bool IsGLContextDependent() const override { return true; }

  dp::GLState const & GetState() const { return m_state; }
  drape_ptr<dp::RenderBucket> && AcceptBuffer() { return move(m_buffer); }

private:
  dp::GLState m_state;
  drape_ptr<dp::RenderBucket> m_buffer;
};

template <typename RenderDataType, Message::Type MessageType>
class FlushRenderDataMessage : public Message
{
public:
  explicit FlushRenderDataMessage(RenderDataType && data) : m_data(std::move(data)) {}

  Type GetType() const override { return MessageType; }
  bool IsGLContextDependent() const override { return true; }

  RenderDataType && AcceptRenderData() { return std::move(m_data); }

private:
  RenderDataType m_data;
};

using FlushOverlaysMessage = FlushRenderDataMessage<TOverlaysRenderData,
                                                    Message::FlushOverlays>;

class InvalidateRectMessage : public Message
{
public:
  InvalidateRectMessage(m2::RectD const & rect)
    : m_rect(rect) {}

  Type GetType() const override { return Message::InvalidateRect; }

  m2::RectD const & GetRect() const { return m_rect; }

private:
  m2::RectD m_rect;
};

class UpdateReadManagerMessage : public Message
{
public:
  UpdateReadManagerMessage(){}

  Type GetType() const override { return Message::UpdateReadManager; }
};

class InvalidateReadManagerRectMessage : public BaseBlockingMessage
{
public:
  InvalidateReadManagerRectMessage(Blocker & blocker, TTilesCollection const & tiles)
    : BaseBlockingMessage(blocker)
    , m_tiles(tiles)
    , m_needRestartReading(false)
  {}

  InvalidateReadManagerRectMessage(Blocker & blocker)
    : BaseBlockingMessage(blocker)
    , m_needRestartReading(true)
  {}

  Type GetType() const override { return Message::InvalidateReadManagerRect; }

  TTilesCollection const & GetTilesForInvalidate() const { return m_tiles; }
  bool NeedRestartReading() const { return m_needRestartReading; }

private:
  TTilesCollection m_tiles;
  bool m_needRestartReading;
};

class ClearUserMarkGroupMessage : public Message
{
public:
  ClearUserMarkGroupMessage(MarkGroupID groupId)
    : m_groupId(groupId)
  {}

  Type GetType() const override { return Message::ClearUserMarkGroup; }

  MarkGroupID GetGroupId() const { return m_groupId; }

private:
  MarkGroupID m_groupId;
};

class ChangeUserMarkGroupVisibilityMessage : public Message
{
public:
  ChangeUserMarkGroupVisibilityMessage(MarkGroupID groupId, bool isVisible)
    : m_groupId(groupId)
    , m_isVisible(isVisible) {}

  Type GetType() const override { return Message::ChangeUserMarkGroupVisibility; }

  MarkGroupID GetGroupId() const { return m_groupId; }
  bool IsVisible() const { return m_isVisible; }

private:
  MarkGroupID m_groupId;
  bool m_isVisible;
};

class UpdateUserMarksMessage : public Message
{
public:
  UpdateUserMarksMessage(drape_ptr<MarkIDCollection> && createdIds,
                         drape_ptr<MarkIDCollection> && removedIds,
                         drape_ptr<UserMarksRenderCollection> && marksRenderParams,
                         drape_ptr<UserLinesRenderCollection> && linesRenderParams)
    : m_createdIds(std::move(createdIds))
    , m_removedIds(std::move(removedIds))
    , m_marksRenderParams(std::move(marksRenderParams))
    , m_linesRenderParams(std::move(linesRenderParams))
  {}

  Type GetType() const override { return Message::UpdateUserMarks; }

  drape_ptr<UserMarksRenderCollection> && AcceptMarkRenderParams() { return std::move(m_marksRenderParams); }
  drape_ptr<UserLinesRenderCollection> && AcceptLineRenderParams() { return std::move(m_linesRenderParams); }
  drape_ptr<MarkIDCollection> && AcceptRemovedIds() { return std::move(m_removedIds); }
  drape_ptr<MarkIDCollection> && AcceptCreatedIds() { return std::move(m_createdIds); }

private:
  drape_ptr<MarkIDCollection> m_createdIds;
  drape_ptr<MarkIDCollection> m_removedIds;
  drape_ptr<UserMarksRenderCollection> m_marksRenderParams;
  drape_ptr<UserLinesRenderCollection> m_linesRenderParams;
};

class UpdateUserMarkGroupMessage : public Message
{
public:
  UpdateUserMarkGroupMessage(MarkGroupID groupId,
                             drape_ptr<MarkIDCollection> && ids)
    : m_groupId(groupId)
    , m_ids(std::move(ids))
  {}

  Type GetType() const override { return Message::UpdateUserMarkGroup; }

  MarkGroupID GetGroupId() const { return m_groupId; }
  drape_ptr<MarkIDCollection> && AcceptIds() { return std::move(m_ids); }

private:
  MarkGroupID m_groupId;
  drape_ptr<MarkIDCollection> m_ids;
};

using FlushUserMarksMessage = FlushRenderDataMessage<TUserMarksRenderData,
                                                     Message::FlushUserMarks>;

class InvalidateUserMarksMessage : public Message
{
public:
  InvalidateUserMarksMessage() = default;

  Type GetType() const override { return Message::InvalidateUserMarks; }
};

class GuiLayerRecachedMessage : public Message
{
public:
  GuiLayerRecachedMessage(drape_ptr<gui::LayerRenderer> && renderer, bool needResetOldGui)
    : m_renderer(move(renderer))
    , m_needResetOldGui(needResetOldGui)
  {}

  Type GetType() const override { return Message::GuiLayerRecached; }
  bool IsGLContextDependent() const override { return true; }

  drape_ptr<gui::LayerRenderer> && AcceptRenderer() { return move(m_renderer); }
  bool NeedResetOldGui() const { return m_needResetOldGui; }

private:
  drape_ptr<gui::LayerRenderer> m_renderer;
  bool const m_needResetOldGui;
};

class GuiRecacheMessage : public Message
{
public:
  GuiRecacheMessage(gui::TWidgetsInitInfo const & initInfo, bool needResetOldGui)
    : m_initInfo(initInfo)
    , m_needResetOldGui(needResetOldGui)
  {}

  Type GetType() const override { return Message::GuiRecache;}
  bool IsGLContextDependent() const override { return true; }

  gui::TWidgetsInitInfo const & GetInitInfo() const { return m_initInfo; }
  bool NeedResetOldGui() const { return m_needResetOldGui; }

private:
  gui::TWidgetsInitInfo m_initInfo;
  bool const m_needResetOldGui;
};

class MapShapesRecacheMessage : public Message
{
public:
  MapShapesRecacheMessage() = default;

  Type GetType() const override { return Message::MapShapesRecache; }
  bool IsGLContextDependent() const override { return true; }
};

class GuiLayerLayoutMessage : public Message
{
public:
  GuiLayerLayoutMessage(gui::TWidgetsLayoutInfo const & info)
    : m_layoutInfo(info)
  {}

  Type GetType() const override { return GuiLayerLayout; }
  bool IsGLContextDependent() const override { return true; }

  gui::TWidgetsLayoutInfo const & GetLayoutInfo() const { return m_layoutInfo; }
  gui::TWidgetsLayoutInfo AcceptLayoutInfo() { return move(m_layoutInfo); }

private:
  gui::TWidgetsLayoutInfo m_layoutInfo;
};

class ShowChoosePositionMarkMessage : public Message
{
public:
  ShowChoosePositionMarkMessage() = default;
  Type GetType() const override { return Message::ShowChoosePositionMark; }
};

class SetKineticScrollEnabledMessage : public Message
{
public:
  SetKineticScrollEnabledMessage(bool enabled)
    : m_enabled(enabled)
  {}

  Type GetType() const override { return Message::SetKineticScrollEnabled; }

  bool IsEnabled() const { return m_enabled; }

private:
  bool m_enabled;
};

class SetAddNewPlaceModeMessage : public Message
{
public:
  SetAddNewPlaceModeMessage(bool enable, vector<m2::TriangleD> && boundArea, bool enableKineticScroll,
                            bool hasPosition, m2::PointD const & position)
    : m_enable(enable)
    , m_boundArea(move(boundArea))
    , m_enableKineticScroll(enableKineticScroll)
    , m_hasPosition(hasPosition)
    , m_position(position)
  {}

  Type GetType() const override { return Message::SetAddNewPlaceMode; }

  vector<m2::TriangleD> && AcceptBoundArea() { return move(m_boundArea); }
  bool IsEnabled() const { return m_enable; }
  bool IsKineticScrollEnabled() const { return m_enableKineticScroll; }
  bool HasPosition() const { return m_hasPosition; }
  m2::PointD const & GetPosition() const { return m_position; }

private:
  bool m_enable;
  vector<m2::TriangleD> m_boundArea;
  bool m_enableKineticScroll;
  bool m_hasPosition;
  m2::PointD m_position;
};

class BlockTapEventsMessage : public Message
{
public:
  BlockTapEventsMessage(bool block)
    : m_needBlock(block)
  {}

  Type GetType() const override { return Message::BlockTapEvents; }

  bool NeedBlock() const { return m_needBlock; }

private:
  bool const m_needBlock;
};

class MapShapesMessage : public Message
{
public:
  MapShapesMessage(drape_ptr<MyPosition> && shape, drape_ptr<SelectionShape> && selection)
    : m_shape(move(shape))
    , m_selection(move(selection))
  {}

  Type GetType() const override { return Message::MapShapes; }
  bool IsGLContextDependent() const override { return true; }

  drape_ptr<MyPosition> && AcceptShape() { return move(m_shape); }
  drape_ptr<SelectionShape> AcceptSelection() { return move(m_selection); }

private:
  drape_ptr<MyPosition> m_shape;
  drape_ptr<SelectionShape> m_selection;
};

class ChangeMyPositionModeMessage : public Message
{
public:
  enum EChangeType
  {
    SwitchNextMode,
    LoseLocation,
    StopFollowing
  };

  explicit ChangeMyPositionModeMessage(EChangeType changeType)
    : m_changeType(changeType)
  {}

  EChangeType GetChangeType() const { return m_changeType; }
  Type GetType() const override { return Message::ChangeMyPositionMode; }

private:
  EChangeType const m_changeType;
};

class CompassInfoMessage : public Message
{
public:
  CompassInfoMessage(location::CompassInfo const & info)
    : m_info(info)
  {}

  Type GetType() const override { return Message::CompassInfo; }

  location::CompassInfo const & GetInfo() const { return m_info; }

private:
  location::CompassInfo const m_info;
};

class GpsInfoMessage : public Message
{
public:
  GpsInfoMessage(location::GpsInfo const & info, bool isNavigable,
                 location::RouteMatchingInfo const & routeInfo)
    : m_info(info)
    , m_isNavigable(isNavigable)
    , m_routeInfo(routeInfo)
  {}

  Type GetType() const override { return Message::GpsInfo; }

  location::GpsInfo const & GetInfo() const { return m_info; }
  bool IsNavigable() const { return m_isNavigable; }
  location::RouteMatchingInfo const & GetRouteInfo() const { return m_routeInfo; }

private:
  location::GpsInfo const m_info;
  bool const m_isNavigable;
  location::RouteMatchingInfo const m_routeInfo;
};

class SelectObjectMessage : public Message
{
public:
  struct DismissTag {};

  SelectObjectMessage(DismissTag)
    : m_selected(SelectionShape::OBJECT_EMPTY)
    , m_glbPoint(m2::PointD::Zero())
    , m_isAnim(false)
    , m_isDismiss(true)
  {}

  SelectObjectMessage(SelectionShape::ESelectedObject selectedObject, m2::PointD const & glbPoint, FeatureID const & featureID,  bool isAnim)
    : m_selected(selectedObject)
    , m_glbPoint(glbPoint)
    , m_featureID(featureID)
    , m_isAnim(isAnim)
    , m_isDismiss(false)
  {}

  Type GetType() const override { return SelectObject; }
  bool IsGLContextDependent() const override { return true; }

  m2::PointD const & GetPosition() const { return m_glbPoint; }
  SelectionShape::ESelectedObject GetSelectedObject() const { return m_selected; }
  FeatureID const & GetFeatureID() const { return m_featureID; }
  bool IsAnim() const { return m_isAnim; }
  bool IsDismiss() const { return m_isDismiss; }

private:
  SelectionShape::ESelectedObject m_selected;
  m2::PointD m_glbPoint;
  FeatureID m_featureID;
  bool m_isAnim;
  bool m_isDismiss;
};

class AddSubrouteMessage : public Message
{
public:
  AddSubrouteMessage(dp::DrapeID subrouteId, SubrouteConstPtr subroute)
    : AddSubrouteMessage(subrouteId, subroute, -1 /* invalid recache id */)
  {}

  AddSubrouteMessage(dp::DrapeID subrouteId, SubrouteConstPtr subroute, int recacheId)
    : m_subrouteId(subrouteId)
    , m_subroute(subroute)
    , m_recacheId(recacheId)
  {}

  Type GetType() const override { return Message::AddSubroute; }

  dp::DrapeID GetSubrouteId() const { return m_subrouteId; };
  SubrouteConstPtr GetSubroute() const { return m_subroute; }
  int GetRecacheId() const { return m_recacheId; }

private:
  dp::DrapeID m_subrouteId;
  SubrouteConstPtr m_subroute;
  int const m_recacheId;
};

class CacheSubrouteArrowsMessage : public Message
{
public:
  CacheSubrouteArrowsMessage(dp::DrapeID subrouteId,
                             std::vector<ArrowBorders> const & borders,
                             int recacheId)
    : m_subrouteId(subrouteId)
    , m_borders(borders)
    , m_recacheId(recacheId)
  {}

  Type GetType() const override { return Message::CacheSubrouteArrows; }
  dp::DrapeID GetSubrouteId() const { return m_subrouteId; }
  std::vector<ArrowBorders> const & GetBorders() const { return m_borders; }
  int GetRecacheId() const { return m_recacheId; }

private:
  dp::DrapeID m_subrouteId;
  std::vector<ArrowBorders> m_borders;
  int const m_recacheId;
};

class RemoveSubrouteMessage : public Message
{
public:
  RemoveSubrouteMessage(dp::DrapeID segmentId, bool deactivateFollowing)
    : m_subrouteId(segmentId)
    , m_deactivateFollowing(deactivateFollowing)
  {}

  Type GetType() const override { return Message::RemoveSubroute; }

  dp::DrapeID GetSegmentId() const { return m_subrouteId; }
  bool NeedDeactivateFollowing() const { return m_deactivateFollowing; }

private:
  dp::DrapeID m_subrouteId;
  bool m_deactivateFollowing;
};

using FlushSubrouteMessage = FlushRenderDataMessage<drape_ptr<SubrouteData>,
                                                    Message::FlushSubroute>;
using FlushSubrouteArrowsMessage = FlushRenderDataMessage<drape_ptr<SubrouteArrowsData>,
                                                          Message::FlushSubrouteArrows>;
using FlushSubrouteMarkersMessage = FlushRenderDataMessage<drape_ptr<SubrouteMarkersData>,
                                                           Message::FlushSubrouteMarkers>;

class AddRoutePreviewSegmentMessage : public Message
{
public:
  AddRoutePreviewSegmentMessage(dp::DrapeID segmentId, m2::PointD const & startPt,
                                m2::PointD const & finishPt)
    : m_segmentId(segmentId)
    , m_startPoint(startPt)
    , m_finishPoint(finishPt)
  {}

  Type GetType() const override { return Message::AddRoutePreviewSegment; }

  dp::DrapeID GetSegmentId() const { return m_segmentId; };
  m2::PointD const & GetStartPoint() const { return m_startPoint; }
  m2::PointD const & GetFinishPoint() const { return m_finishPoint; }

private:
  dp::DrapeID m_segmentId;
  m2::PointD m_startPoint;
  m2::PointD m_finishPoint;
};

class RemoveRoutePreviewSegmentMessage : public Message
{
public:
  RemoveRoutePreviewSegmentMessage()
    : m_needRemoveAll(true)
  {}

  explicit RemoveRoutePreviewSegmentMessage(dp::DrapeID segmentId)
    : m_segmentId(segmentId)
    , m_needRemoveAll(false)
  {}

  Type GetType() const override { return Message::RemoveRoutePreviewSegment; }

  dp::DrapeID GetSegmentId() const { return m_segmentId; }
  bool NeedRemoveAll() const { return m_needRemoveAll; }

private:
  dp::DrapeID m_segmentId;
  bool m_needRemoveAll;
};

class SetSubrouteVisibilityMessage : public Message
{
public:
  SetSubrouteVisibilityMessage(dp::DrapeID subrouteId, bool isVisible)
    : m_subrouteId(subrouteId)
    , m_isVisible(isVisible)
  {}

  Type GetType() const override { return Message::SetSubrouteVisibility; }

  dp::DrapeID GetSubrouteId() const { return m_subrouteId; }
  bool IsVisible() const { return m_isVisible; }

private:
  dp::DrapeID m_subrouteId;
  bool m_isVisible;
};

class UpdateMapStyleMessage : public BaseBlockingMessage
{
public:
  UpdateMapStyleMessage(Blocker & blocker)
    : BaseBlockingMessage(blocker)
  {}

  Type GetType() const override { return Message::UpdateMapStyle; }
};

class FollowRouteMessage : public Message
{
public:
  FollowRouteMessage(int preferredZoomLevel, int preferredZoomLevelIn3d, bool enableAutoZoom)
    : m_preferredZoomLevel(preferredZoomLevel)
    , m_preferredZoomLevelIn3d(preferredZoomLevelIn3d)
    , m_enableAutoZoom(enableAutoZoom)
  {}

  Type GetType() const override { return Message::FollowRoute; }

  int GetPreferredZoomLevel() const { return m_preferredZoomLevel; }
  int GetPreferredZoomLevelIn3d() const { return m_preferredZoomLevelIn3d; }
  bool EnableAutoZoom() const { return m_enableAutoZoom; }

private:
  int const m_preferredZoomLevel;
  int const m_preferredZoomLevelIn3d;
  bool const m_enableAutoZoom;
};

class SwitchMapStyleMessage : public BaseBlockingMessage
{
public:
  SwitchMapStyleMessage(Blocker & blocker)
    : BaseBlockingMessage(blocker)
  {}

  Type GetType() const override { return Message::SwitchMapStyle; }
};

class InvalidateMessage : public Message
{
public:
  InvalidateMessage(){}

  Type GetType() const override { return Message::Invalidate; }
};

class RecoverGLResourcesMessage : public Message
{
public:
  RecoverGLResourcesMessage(){}

  Type GetType() const override { return Message::RecoverGLResources; }
  bool IsGLContextDependent() const override { return true; }
};

class SetVisibleViewportMessage : public Message
{
public:
  SetVisibleViewportMessage(m2::RectD const & rect)
    : m_rect(rect)
  {}

  Type GetType() const override { return Message::SetVisibleViewport;  }

  m2::RectD const &  GetRect() const { return m_rect; }

private:
  m2::RectD m_rect;
};

class DeactivateRouteFollowingMessage : public Message
{
public:
  DeactivateRouteFollowingMessage(){}

  Type GetType() const override { return Message::DeactivateRouteFollowing; }
};

class Allow3dModeMessage : public Message
{
public:
  Allow3dModeMessage(bool allowPerspective, bool allow3dBuildings)
    : m_allowPerspective(allowPerspective)
    , m_allow3dBuildings(allow3dBuildings)
  {}

  Type GetType() const override { return Message::Allow3dMode; }

  bool AllowPerspective() const { return m_allowPerspective; }
  bool Allow3dBuildings() const { return m_allow3dBuildings; }

private:
  bool const m_allowPerspective;
  bool const m_allow3dBuildings;
};

class AllowAutoZoomMessage : public Message
{
public:
  AllowAutoZoomMessage(bool allowAutoZoom)
    : m_allowAutoZoom(allowAutoZoom)
  {}

  Type GetType() const override { return Message::AllowAutoZoom; }

  bool AllowAutoZoom() const { return m_allowAutoZoom; }

private:
  bool const m_allowAutoZoom;
};

class Allow3dBuildingsMessage : public Message
{
public:
  Allow3dBuildingsMessage(bool allow3dBuildings)
    : m_allow3dBuildings(allow3dBuildings)
  {}

  Type GetType() const override { return Message::Allow3dBuildings; }

  bool Allow3dBuildings() const { return m_allow3dBuildings; }

private:
  bool const m_allow3dBuildings;
};

class EnablePerspectiveMessage : public Message
{
public:
  EnablePerspectiveMessage() = default;

  Type GetType() const override { return Message::EnablePerspective; }
};

class CacheCirclesPackMessage : public Message
{
public:
  enum Destination
  {
    GpsTrack,
    RoutePreview
  };

  CacheCirclesPackMessage(uint32_t pointsCount, Destination dest)
    : m_pointsCount(pointsCount)
    , m_destination(dest)
  {}

  Type GetType() const override { return Message::CacheCirclesPack; }

  uint32_t GetPointsCount() const { return m_pointsCount; }
  Destination GetDestination() const { return m_destination; }

private:
  uint32_t m_pointsCount;
  Destination m_destination;
};

using BaseFlushCirclesPackMessage = FlushRenderDataMessage<drape_ptr<CirclesPackRenderData>,
                                                           Message::FlushCirclesPack>;
class FlushCirclesPackMessage : public BaseFlushCirclesPackMessage
{
public:
  FlushCirclesPackMessage(drape_ptr<CirclesPackRenderData> && renderData,
                          CacheCirclesPackMessage::Destination dest)
    : BaseFlushCirclesPackMessage(std::move(renderData))
    , m_destination(dest)
  {}

  CacheCirclesPackMessage::Destination GetDestination() const { return m_destination; }

private:
  CacheCirclesPackMessage::Destination m_destination;
};

class UpdateGpsTrackPointsMessage : public Message
{
public:
  UpdateGpsTrackPointsMessage(std::vector<GpsTrackPoint> && toAdd,
                              std::vector<uint32_t> && toRemove)
    : m_pointsToAdd(std::move(toAdd))
    , m_pointsToRemove(std::move(toRemove))
  {}

  Type GetType() const override { return Message::UpdateGpsTrackPoints; }

  std::vector<GpsTrackPoint> const & GetPointsToAdd() { return m_pointsToAdd; }
  std::vector<uint32_t> const & GetPointsToRemove() { return m_pointsToRemove; }

private:
  std::vector<GpsTrackPoint> m_pointsToAdd;
  std::vector<uint32_t> m_pointsToRemove;
};

class ClearGpsTrackPointsMessage : public Message
{
public:
  ClearGpsTrackPointsMessage() = default;

  Type GetType() const override { return Message::ClearGpsTrackPoints; }
};

class SetTimeInBackgroundMessage : public Message
{
public:
  explicit SetTimeInBackgroundMessage(double time)
    : m_time(time)
  {}

  Type GetType() const override { return Message::SetTimeInBackground; }

  double GetTime() const { return m_time; }

private:
  double m_time;
};

class SetDisplacementModeMessage : public Message
{
public:
  explicit SetDisplacementModeMessage(int mode)
    : m_mode(mode)
  {}

  Type GetType() const override { return Message::SetDisplacementMode; }

  int GetMode() const { return m_mode; }

private:
  int m_mode;
};

class RequestSymbolsSizeMessage : public Message
{
public:
  using TRequestSymbolsSizeCallback = function<void(vector<m2::PointF> const &)>;

  RequestSymbolsSizeMessage(vector<string> const & symbols,
                            TRequestSymbolsSizeCallback const & callback)
    : m_symbols(symbols)
    , m_callback(callback)
  {}

  Type GetType() const override { return Message::RequestSymbolsSize; }

  vector<string> const & GetSymbols() const { return m_symbols; }

  void InvokeCallback(vector<m2::PointF> const & sizes)
  {
    if (m_callback != nullptr)
      m_callback(sizes);
  }

private:
  vector<string> m_symbols;
  TRequestSymbolsSizeCallback m_callback;
};

class EnableTrafficMessage : public Message
{
public:
  explicit EnableTrafficMessage(bool trafficEnabled)
    : m_trafficEnabled(trafficEnabled)
  {}

  Type GetType() const override { return Message::EnableTraffic; }

  bool IsTrafficEnabled() const { return m_trafficEnabled; }

private:
  bool const m_trafficEnabled;
};

class FlushTrafficGeometryMessage : public BaseTileMessage
{
public:
  FlushTrafficGeometryMessage(TileKey const & tileKey, TrafficSegmentsGeometry && segments)
    : BaseTileMessage(tileKey)
    , m_segments(move(segments))
  {}

  Type GetType() const override { return Message::FlushTrafficGeometry; }

  TrafficSegmentsGeometry & GetSegments() { return m_segments; }

private:
  TrafficSegmentsGeometry m_segments;
};

class RegenerateTrafficMessage : public Message
{
public:
  Type GetType() const override { return Message::RegenerateTraffic; }
};

class UpdateTrafficMessage : public Message
{
public:
  explicit UpdateTrafficMessage(TrafficSegmentsColoring && segmentsColoring)
    : m_segmentsColoring(move(segmentsColoring))
  {}

  Type GetType() const override { return Message::UpdateTraffic; }

  TrafficSegmentsColoring & GetSegmentsColoring() { return m_segmentsColoring; }

private:
  TrafficSegmentsColoring m_segmentsColoring;
};

using FlushTrafficDataMessage = FlushRenderDataMessage<TrafficRenderData,
                                                       Message::FlushTrafficData>;

class ClearTrafficDataMessage : public Message
{
public:
  explicit ClearTrafficDataMessage(MwmSet::MwmId const & mwmId)
    : m_mwmId(mwmId)
  {}

  Type GetType() const override { return Message::ClearTrafficData; }

  MwmSet::MwmId const & GetMwmId() { return m_mwmId; }

private:
  MwmSet::MwmId m_mwmId;
};

class SetSimplifiedTrafficColorsMessage : public Message
{
public:
  SetSimplifiedTrafficColorsMessage(bool isSimplified)
    : m_isSimplified(isSimplified)
  {}

  Type GetType() const override { return Message::SetSimplifiedTrafficColors; }

  bool IsSimplified() const { return m_isSimplified; }

private:
  bool const m_isSimplified;
};

class DrapeApiAddLinesMessage : public Message
{
public:
  explicit DrapeApiAddLinesMessage(DrapeApi::TLines const & lines)
    : m_lines(lines)
  {}

  Type GetType() const override { return Message::DrapeApiAddLines; }

  DrapeApi::TLines const & GetLines() const { return m_lines; }

private:
  DrapeApi::TLines m_lines;
};

class DrapeApiRemoveMessage : public Message
{
public:
  explicit DrapeApiRemoveMessage(string const & id, bool removeAll = false)
    : m_id(id)
    , m_removeAll(removeAll)
  {}

  Type GetType() const override { return Message::DrapeApiRemove; }

  string const & GetId() const { return m_id; }
  bool NeedRemoveAll() const { return m_removeAll; }

private:
  string m_id;
  bool m_removeAll;
};

class DrapeApiFlushMessage : public Message
{
public:
  using TProperties = vector<drape_ptr<DrapeApiRenderProperty>>;

  explicit DrapeApiFlushMessage(TProperties && properties)
    : m_properties(move(properties))
  {}

  Type GetType() const override { return Message::DrapeApiFlush; }

  TProperties && AcceptProperties() { return move(m_properties); }

private:
  TProperties m_properties;
};

class SetCustomFeaturesMessage : public Message
{
public:
  explicit SetCustomFeaturesMessage(std::set<FeatureID> && ids)
    : m_features(std::move(ids))
  {}

  Type GetType() const override { return Message::SetCustomFeatures; }

  std::set<FeatureID> && AcceptFeatures() { return std::move(m_features); }

private:
  std::set<FeatureID> m_features;
};

class RemoveCustomFeaturesMessage : public Message
{
public:
  RemoveCustomFeaturesMessage() = default;
  explicit RemoveCustomFeaturesMessage(MwmSet::MwmId const & mwmId)
    : m_mwmId(mwmId), m_removeAll(false)
  {}

  Type GetType() const override { return Message::RemoveCustomFeatures; }
  bool NeedRemoveAll() const { return m_removeAll; }
  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }

private:
  MwmSet::MwmId m_mwmId;
  bool m_removeAll = true;
};

class UpdateCustomFeaturesMessage : public Message
{
public:
  explicit UpdateCustomFeaturesMessage(std::vector<FeatureID> && features)
    : m_features(std::move(features))
  {}

  Type GetType() const override { return Message::UpdateCustomFeatures; }

  std::vector<FeatureID> && AcceptFeatures() { return std::move(m_features); }

private:
  std::vector<FeatureID> m_features;
};

class SetPostprocessStaticTexturesMessage : public Message
{
public:
  explicit SetPostprocessStaticTexturesMessage(drape_ptr<PostprocessStaticTextures> && textures)
    : m_textures(std::move(textures))
  {}

  Type GetType() const override { return Message::SetPostprocessStaticTextures; }
  bool IsGLContextDependent() const override { return true; }

  drape_ptr<PostprocessStaticTextures> && AcceptTextures() { return std::move(m_textures); }

private:
  drape_ptr<PostprocessStaticTextures> m_textures;
};

class SetPosteffectEnabledMessage : public Message
{
public:
  SetPosteffectEnabledMessage(PostprocessRenderer::Effect effect, bool enabled)
    : m_effect(effect)
    , m_enabled(enabled)
  {}

  Type GetType() const override { return Message::SetPosteffectEnabled; }
  PostprocessRenderer::Effect GetEffect() const { return m_effect; }
  bool IsEnabled() const { return m_enabled; }

private:
  PostprocessRenderer::Effect const m_effect;
  bool const m_enabled;
};

class RunFirstLaunchAnimationMessage : public Message
{
public:
  Type GetType() const override { return Message::RunFirstLaunchAnimation; }
};

class UpdateMetalinesMessage : public Message
{
public:
  Type GetType() const override { return Message::UpdateMetalines; }
};

class PostUserEventMessage : public Message
{
public:
  PostUserEventMessage(drape_ptr<UserEvent> && event)
    : m_event(std::move(event))
  {}

  Type GetType() const override { return Message::PostUserEvent; }

  drape_ptr<UserEvent> && AcceptEvent() { return std::move(m_event); }

private:
  drape_ptr<UserEvent> m_event;
};

class FinishTexturesInitializationMessage : public Message
{
public:
  Type GetType() const override { return Message::FinishTexturesInitialization; }
};
}  // namespace df
