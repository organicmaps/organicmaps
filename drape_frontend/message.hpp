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
    AddRoute,
    CacheRouteSign,
    CacheRouteArrows,
    RemoveRoute,
    FlushRoute,
    FlushRouteSign,
    FlushRouteArrows,
    FollowRoute,
    DeactivateRouteFollowing,
    UpdateMapStyle,
    InvalidateTextures,
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
    AddTrafficSegments,
    SetTrafficTexCoords,
    UpdateTraffic,
    FlushTrafficData,
    DrapeApiAddLines,
    DrapeApiRemove,
    DrapeApiFlush,
  };

  virtual ~Message() {}
  virtual Type GetType() const { return Unknown; }
  virtual bool IsGLContextDependent() const { return false; }
};

enum class MessagePriority
{
  Normal,
  High,
  UberHighSingleton
};

} // namespace df
