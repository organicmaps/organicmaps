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
    ClearUserMarkLayer,
    ChangeUserMarkLayerVisibility,
    UpdateUserMarkLayer,
    FlushUserMarks,
    GuiLayerRecached,
    GuiRecache,
    GuiLayerLayout,
    MapShapesRecache,
    MapShapes,
    ChangeMyPostitionMode,
    CompassInfo,
    GpsInfo,
    FindVisiblePOI,
    SelectObject,
    GetSelectedObject,
    GetMyPosition,
    AddRouteSegment,
    RemoveRouteSegment,
    CacheRouteArrows,
    FlushRoute,
    FlushRouteArrows,
    FollowRoute,
    DeactivateRouteFollowing,
    UpdateMapStyle,
    SwitchMapStyle,
    Invalidate,
    Allow3dMode,
    Allow3dBuildings,
    EnablePerspective,
    CacheGpsTrackPoints,
    FlushGpsTrackPoints,
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
    AddCustomSymbols,
    RemoveCustomSymbols,
    UpdateCustomSymbols,
    SetPostprocessStaticTextures,
    SetPosteffectEnabled,
    RunFirstLaunchAnimation,
  };

  virtual ~Message() {}
  virtual Type GetType() const { return Unknown; }
  virtual bool IsGLContextDependent() const { return false; }
};

enum class MessagePriority
{
  Normal,
  High,
  UberHighSingleton,
  Low
};

} // namespace df
