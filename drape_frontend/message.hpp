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
    GuiLayerRecached,
    GuiRecache,
    GuiLayerLayout,
    MyPositionShape,
    ChangeMyPostitionMode,
    CompassInfo,
    GpsInfo,
    FindVisiblePOI,
    SelectObject,
    GetSelectedObject,
    GetMyPosition,
    AddRoute,
    CacheRouteSign,
    RemoveRoute,
    FlushRoute,
    FlushRouteSign,
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
    BlockTapEvents
  };

  virtual ~Message() {}
  virtual Type GetType() const { return Unknown; }
};

enum class MessagePriority
{
  Normal,
  High,
  UberHighSingleton
};

} // namespace df
