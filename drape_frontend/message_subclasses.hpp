#pragma once

#include "message.hpp"
#include "viewport.hpp"
#include "tile_key.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../drape/glstate.hpp"
#include "../drape/pointers.hpp"
#include "../drape/render_bucket.hpp"

namespace df
{
  class BaseTileMessage : public Message
  {
  public:
    BaseTileMessage(const TileKey & key, Message::Type type)
      : m_tileKey(key)
    {
      SetType(type);
    }

    const TileKey & GetKey() const { return m_tileKey; }

  private:
    TileKey m_tileKey;
  };

  class TileReadStartMessage : public BaseTileMessage
  {
  public:
    TileReadStartMessage(const TileKey & key)
      : BaseTileMessage(key, Message::TileReadStarted) {}
  };

  class TileReadEndMessage : public BaseTileMessage
  {
  public:
    TileReadEndMessage(const TileKey & key)
      : BaseTileMessage(key, Message::TileReadEnded) {}
  };

  class FlushRenderBucketMessage : public BaseTileMessage
  {
  public:
    FlushRenderBucketMessage(const TileKey & key, const GLState state, TransferPointer<RenderBucket> buffer)
      : BaseTileMessage(key, Message::FlushTile)
      , m_state(state)
      , m_buffer(buffer)
    {
    }

    ~FlushRenderBucketMessage()
    {
      m_buffer.Destroy();
    }

    const GLState & GetState() const { return m_state; }
    MasterPointer<RenderBucket> AcceptBuffer() { return MasterPointer<RenderBucket>(m_buffer); }

  private:
    GLState m_state;
    TransferPointer<RenderBucket> m_buffer;
  };

  class ResizeMessage : public Message
  {
  public:
    ResizeMessage(Viewport const & viewport) : m_viewport(viewport)
    {
      SetType(Resize);
    }

    const Viewport & GetViewport() const { return m_viewport; }

  private:
    Viewport m_viewport;
  };

  class UpdateCoverageMessage : public Message
  {
  public:
    UpdateCoverageMessage(ScreenBase const & screen) : m_screen(screen)
    {
      SetType(UpdateCoverage);
    }
    const ScreenBase & GetScreen() const { return m_screen; }

  private:
    ScreenBase m_screen;
  };
}
