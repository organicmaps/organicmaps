#pragma once

#include "drape_frontend/message.hpp"
#include "drape_frontend/viewport.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/user_marks_provider.hpp"

#include "drape_gui/layer_render.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"


#include "drape/glstate.hpp"
#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"

#include "std/shared_ptr.hpp"
#include "std/set.hpp"

namespace df
{

class BaseTileMessage : public Message
{
public:
  BaseTileMessage(TileKey const & key, Message::Type type)
    : m_tileKey(key)
  {
    SetType(type);
  }

  TileKey const & GetKey() const { return m_tileKey; }

private:
  TileKey m_tileKey;
};

class TileReadStartMessage : public BaseTileMessage
{
public:
  TileReadStartMessage(TileKey const & key)
    : BaseTileMessage(key, Message::TileReadStarted) {}
};

class TileReadEndMessage : public BaseTileMessage
{
public:
  TileReadEndMessage(TileKey const & key)
    : BaseTileMessage(key, Message::TileReadEnded) {}
};

class FlushRenderBucketMessage : public BaseTileMessage
{
public:
  FlushRenderBucketMessage(TileKey const & key, dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> buffer)
    : BaseTileMessage(key, Message::FlushTile)
    , m_state(state)
    , m_buffer(buffer)
  {
  }

  ~FlushRenderBucketMessage()
  {
    m_buffer.Destroy();
  }

  dp::GLState const & GetState() const { return m_state; }
  dp::MasterPointer<dp::RenderBucket> AcceptBuffer() { return dp::MasterPointer<dp::RenderBucket>(m_buffer); }

private:
  dp::GLState m_state;
  dp::TransferPointer<dp::RenderBucket> m_buffer;
};

class ResizeMessage : public Message
{
public:
  ResizeMessage(Viewport const & viewport) : m_viewport(viewport)
  {
    SetType(Resize);
  }

  Viewport const & GetViewport() const { return m_viewport; }

private:
  Viewport m_viewport;
};

class UpdateModelViewMessage : public Message
{
public:
  UpdateModelViewMessage(ScreenBase const & screen)
    : m_screen(screen)
  {
    SetType(UpdateModelView);
  }

  ScreenBase const & GetScreen() const { return m_screen; }

private:
  ScreenBase m_screen;
};

class InvalidateRectMessage : public Message
{
public:
  InvalidateRectMessage(m2::RectD const & rect)
    : m_rect(rect)
  {
    SetType(InvalidateRect);
  }

  m2::RectD const & GetRect() const { return m_rect; }

private:
  m2::RectD m_rect;
};

class UpdateReadManagerMessage : public UpdateModelViewMessage
{
public:
  UpdateReadManagerMessage(ScreenBase const & screen, set<TileKey> const & tiles)
    : UpdateModelViewMessage(screen)
    , m_tiles(tiles)
  {
    SetType(UpdateReadManager);
  }

  set<TileKey> const & GetTiles() const { return m_tiles; }

private:
  set<TileKey> m_tiles;
};

class InvalidateReadManagerRectMessage : public Message
{
public:
  InvalidateReadManagerRectMessage(set<TileKey> const & tiles)
    : m_tiles(tiles)
  {
    SetType(InvalidateReadManagerRect);
  }

  set<TileKey> const & GetTilesForInvalidate() const { return m_tiles; }

private:
  set<TileKey> m_tiles;
};

template <typename T>
T * CastMessage(dp::RefPointer<Message> msg)
{
  return static_cast<T *>(msg.GetRaw());
}

class ClearUserMarkLayerMessage : public BaseTileMessage
{
public:
  ClearUserMarkLayerMessage(TileKey const & tileKey)
    : BaseTileMessage(tileKey, Message::ClearUserMarkLayer) {}
};

class ChangeUserMarkLayerVisibilityMessage : public BaseTileMessage
{
public:
  ChangeUserMarkLayerVisibilityMessage(TileKey const & tileKey, bool isVisible)
    : BaseTileMessage(tileKey, Message::ChangeUserMarkLayerVisibility)
    , m_isVisible(isVisible) {}

bool IsVisible() const { return m_isVisible; }

private:
  bool m_isVisible;
};

class UpdateUserMarkLayerMessage : public BaseTileMessage
{
public:
  UpdateUserMarkLayerMessage(TileKey const & tileKey, UserMarksProvider * provider)
    : BaseTileMessage(tileKey, Message::UpdateUserMarkLayer)
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
  GuiLayerRecachedMessage(dp::TransferPointer<gui::LayerRenderer> renderer)
    : m_renderer(move(renderer))
  {
    SetType(GuiLayerRecached);
  }

  dp::MasterPointer<gui::LayerRenderer> AcceptRenderer()
  {
    return dp::MasterPointer<gui::LayerRenderer>(m_renderer);
  }

private:
  dp::TransferPointer<gui::LayerRenderer> m_renderer;
};

} // namespace df
