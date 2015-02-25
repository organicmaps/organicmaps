#pragma once

namespace df
{

class Message
{
public:
  enum Type
  {
    // in perfect world GetType never return this type
    // for this you need call SetType on subclass constructor
    Unknown,
    TileReadStarted,
    TileReadEnded,
    FlushTile,
    MapShapeReaded,
    UpdateModelView,
    UpdateReadManager,
    InvalidateRect,
    InvalidateReadManagerRect,
    Resize,
    ClearUserMarkLayer,
    ChangeUserMarkLayerVisibility,
    UpdateUserMarkLayer,
    GuiLayerRecached
  };

  Message();
  virtual ~Message() {}
  Type GetType() const;

protected:
  void SetType(Type t);

private:
  Type m_type;
};

} // namespace df
