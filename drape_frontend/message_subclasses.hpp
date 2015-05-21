#pragma once

#include "drape_frontend/my_position.hpp"
#include "drape_frontend/message.hpp"
#include "drape_frontend/viewport.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/user_marks_provider.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "drape_gui/country_status_helper.hpp"
#include "drape_gui/layer_render.hpp"
#include "drape_gui/skin.hpp"

#include "drape/glstate.hpp"
#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"

#include "platform/location.hpp"

#include "std/shared_ptr.hpp"
#include "std/set.hpp"
#include "std/function.hpp"
#include "std/utility.hpp"

namespace df
{

class BaseTileMessage : public Message
{
public:
  BaseTileMessage(TileKey const & key)
    : m_tileKey(key) {}

  TileKey const & GetKey() const { return m_tileKey; }

private:
  TileKey m_tileKey;
};

class TileReadStartMessage : public BaseTileMessage
{
public:
  TileReadStartMessage(TileKey const & key)
    : BaseTileMessage(key) {}

  Type GetType() const override { return Message::TileReadStarted; }
};

class TileReadEndMessage : public BaseTileMessage
{
public:
  TileReadEndMessage(TileKey const & key)
    : BaseTileMessage(key) {}

  Type GetType() const override { return Message::TileReadEnded; }
};

class FinishReadingMessage : public Message
{
public:
  template<typename T> FinishReadingMessage(T && tiles)
    : m_tiles(forward<T>(tiles))
  {}

  Type GetType() const override { return Message::FinishReading; }

  TTilesCollection const & GetTiles() { return m_tiles; }
  TTilesCollection && MoveTiles() { return move(m_tiles); }

private:
  TTilesCollection m_tiles;
};

class FlushRenderBucketMessage : public BaseTileMessage
{
public:
  FlushRenderBucketMessage(TileKey const & key, dp::GLState const & state, drape_ptr<dp::RenderBucket> && buffer)
    : BaseTileMessage(key)
    , m_state(state)
    , m_buffer(move(buffer))
  {}

  Type GetType() const override { return Message::FlushTile; }

  dp::GLState const & GetState() const { return m_state; }
  drape_ptr<dp::RenderBucket> && AcceptBuffer() { return move(m_buffer); }

private:
  dp::GLState m_state;
  drape_ptr<dp::RenderBucket> m_buffer;
};

class ResizeMessage : public Message
{
public:
  ResizeMessage(Viewport const & viewport)
    : m_viewport(viewport) {}

  Type GetType() const override { return Message::Resize; }

  Viewport const & GetViewport() const { return m_viewport; }

private:
  Viewport m_viewport;
};

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
  UpdateReadManagerMessage(ScreenBase const & screen, TTilesCollection && tiles)
    : m_tiles(move(tiles))
    , m_screen(screen)
  {}

  Type GetType() const override { return Message::UpdateReadManager; }

  TTilesCollection const & GetTiles() const { return m_tiles; }
  ScreenBase const & GetScreen() const { return m_screen; }

private:
  TTilesCollection m_tiles;
  ScreenBase m_screen;
};

class InvalidateReadManagerRectMessage : public Message
{
public:
  InvalidateReadManagerRectMessage(TTilesCollection const & tiles)
    : m_tiles(tiles) {}

  Type GetType() const override { return Message::InvalidateReadManagerRect; }

  TTilesCollection const & GetTilesForInvalidate() const { return m_tiles; }

private:
  TTilesCollection m_tiles;
};

class ClearUserMarkLayerMessage : public BaseTileMessage
{
public:
  ClearUserMarkLayerMessage(TileKey const & tileKey)
    : BaseTileMessage(tileKey) {}

  Type GetType() const override { return Message::ClearUserMarkLayer; }
};

class ChangeUserMarkLayerVisibilityMessage : public BaseTileMessage
{
public:
  ChangeUserMarkLayerVisibilityMessage(TileKey const & tileKey, bool isVisible)
    : BaseTileMessage(tileKey)
    , m_isVisible(isVisible) {}

  Type GetType() const override { return Message::ChangeUserMarkLayerVisibility; }

  bool IsVisible() const { return m_isVisible; }

private:
  bool m_isVisible;
};

class UpdateUserMarkLayerMessage : public BaseTileMessage
{
public:
  UpdateUserMarkLayerMessage(TileKey const & tileKey, UserMarksProvider * provider)
    : BaseTileMessage(tileKey)
    , m_provider(provider)
  {
    m_provider->IncrementCounter();
  }

  ~UpdateUserMarkLayerMessage()
  {
    ASSERT(m_inProcess == false, ());
    m_provider->DecrementCounter();
    if (m_provider->IsPendingOnDelete() && m_provider->CanBeDeleted())
      delete m_provider;
  }

  Type GetType() const override { return Message::UpdateUserMarkLayer; }

  UserMarksProvider const * StartProcess()
  {
    m_provider->BeginRead();
#ifdef DEBUG
    m_inProcess = true;
#endif
    return m_provider;
  }

  void EndProcess()
  {
#ifdef DEBUG
    m_inProcess = false;
#endif
    m_provider->EndRead();
  }

private:
  UserMarksProvider * m_provider;
#ifdef DEBUG
  bool m_inProcess;
#endif
};

class GuiLayerRecachedMessage : public Message
{
public:
  GuiLayerRecachedMessage(drape_ptr<gui::LayerRenderer> && renderer)
    : m_renderer(move(renderer)) {}

  Type GetType() const override { return Message::GuiLayerRecached; }

  drape_ptr<gui::LayerRenderer> && AcceptRenderer() { return move(m_renderer); }

private:
  drape_ptr<gui::LayerRenderer> m_renderer;
};

class GuiRecacheMessage : public Message
{
public:
  GuiRecacheMessage(gui::Skin::ElementName elements)
    : m_elements(elements)
  {
  }

  Type GetType() const override { return Message::GuiRecache;}
  gui::Skin::ElementName GetElements() const { return m_elements; }

private:
  gui::Skin::ElementName m_elements;
};

class CountryInfoUpdateMessage : public Message
{
public:
  CountryInfoUpdateMessage(gui::CountryInfo const & info, bool isCurrentCountry,
                           bool isCountryLoaded)
    : m_countryInfo(info)
    , m_isCurrentCountry(isCurrentCountry)
    , m_isCountryLoaded(isCountryLoaded)
  {}

  Type GetType() const override { return Message::CountryInfoUpdate;}
  gui::CountryInfo const & GetCountryInfo() const { return m_countryInfo; }
  bool IsCurrentCountry() const { return m_isCurrentCountry; }
  bool IsCountryLoaded() const { return m_isCountryLoaded; }

private:
  gui::CountryInfo m_countryInfo;
  bool m_isCurrentCountry;
  bool m_isCountryLoaded;
};

class MyPositionShapeMessage : public Message
{
public:
  MyPositionShapeMessage(drape_ptr<MyPosition> && shape)
    : m_shape(move(shape))
  {}

  Type GetType() const override { return Message::MyPositionShape; }

  drape_ptr<MyPosition> && AcceptShape() { return move(m_shape); }

private:
  drape_ptr<MyPosition> m_shape;
};

class StopRenderingMessage : public Message
{
public:
  StopRenderingMessage(){}
  Type GetType() const override { return Message::StopRendering; }
};

class ChangeMyPositionModeMessage : public Message
{
public:
  enum EChangeType
  {
    TYPE_NEXT,
    TYPE_CANCEL,
    TYPE_INVALIDATE
  };

  ChangeMyPositionModeMessage(EChangeType changeType)
    : m_changeType(changeType)
  {
  }

  EChangeType GetChangeType() const { return m_changeType; }
  Type GetType() const override { return Message::ChangeMyPostitionMode; }

private:
  EChangeType m_changeType;
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
  location::CompassInfo m_info;
};

class GpsInfoMessage : public Message
{
public:
  GpsInfoMessage(location::GpsInfo const & info, bool isNavigable, location::RouteMatchingInfo const & routeInfo)
    : m_info(info)
    , m_isNavigable(isNavigable)
    , m_routeInfo(routeInfo)
  {}

  Type GetType() const override { return Message::GpsInfo; }
  location::GpsInfo const & GetInfo() const { return m_info; }
  bool IsNavigable() const { return m_isNavigable; }
  location::RouteMatchingInfo const & GetRouteInfo() const { return m_routeInfo; }

private:
  location::GpsInfo m_info;
  bool m_isNavigable;
  location::RouteMatchingInfo m_routeInfo;
};

} // namespace df
