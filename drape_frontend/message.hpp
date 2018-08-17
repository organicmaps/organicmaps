#pragma once

namespace df
{
class Message
{
public:
  enum Type
  {
    Unknown,
    TileReadStarted,
    TileReadEnded,
    FinishReading,
    FinishTileRead,
    FlushTile,
    FlushOverlays,
    MapShapeReaded,
    OverlayMapShapeReaded,
    UpdateReadManager,
    InvalidateRect,
    InvalidateReadManagerRect,
    UpdateUserMarkGroup,
    ClearUserMarkGroup,
    ChangeUserMarkGroupVisibility,
    UpdateUserMarks,
    InvalidateUserMarks,
    FlushUserMarks,
    GuiLayerRecached,
    GuiRecache,
    GuiLayerLayout,
    MapShapesRecache,
    MapShapes,
    ChangeMyPositionMode,
    CompassInfo,
    GpsInfo,
    SelectObject,
    AddSubroute,
    RemoveSubroute,
    CacheSubrouteArrows,
    FlushSubroute,
    FlushSubrouteArrows,
    FlushSubrouteMarkers,
    FollowRoute,
    DeactivateRouteFollowing,
    SetSubrouteVisibility,
    AddRoutePreviewSegment,
    RemoveRoutePreviewSegment,
    UpdateMapStyle,
    SwitchMapStyle,
    Invalidate,
    Allow3dMode,
    Allow3dBuildings,
    EnablePerspective,
    FlushCirclesPack,
    CacheCirclesPack,
    UpdateGpsTrackPoints,
    ClearGpsTrackPoints,
    ShowChoosePositionMark,
    SetKineticScrollEnabled,
    BlockTapEvents,
    SetTimeInBackground,
    SetAddNewPlaceMode,
    SetDisplacementMode,
    AllowAutoZoom,
    RequestSymbolsSize,
    RecoverGLResources,
    SetVisibleViewport,
    EnableTraffic,
    FlushTrafficGeometry,
    RegenerateTraffic,
    UpdateTraffic,
    FlushTrafficData,
    ClearTrafficData,
    SetSimplifiedTrafficColors,
    DrapeApiAddLines,
    DrapeApiRemove,
    DrapeApiFlush,
    SetCustomFeatures,
    RemoveCustomFeatures,
    SetTrackedFeatures,
    SetPostprocessStaticTextures,
    SetPosteffectEnabled,
    RunFirstLaunchAnimation,
    UpdateMetalines,
    PostUserEvent,
    FinishTexturesInitialization,
    EnableUGCRendering,
    EnableDebugRectRendering,
    EnableTransitScheme,
    UpdateTransitScheme,
    ClearTransitSchemeData,
    ClearAllTransitSchemeData,
    RegenerateTransitScheme,
    FlushTransitScheme,
    ShowDebugInfo,
    NotifyRenderThread
  };

  virtual ~Message() = default;
  virtual Type GetType() const { return Unknown; }
  virtual bool IsGLContextDependent() const { return false; }
};

enum class MessagePriority
{
  // This is standard priority. It must be used for majority of messages.
  // This priority guarantees order of messages processing.
  Normal,
  // This priority is used for system messages where order of processing
  // could be neglected, so it does not guarantee order of messages processing.
  // Also it must be used for messages which stop threads.
  High,
  // It can be used for the only system message (UpdateReadManagerMessage) and
  // must not be used anywhere else.
  UberHighSingleton,
  // This priority allows to process messages after any other messages in queue.
  Low
};
}  // namespace df
