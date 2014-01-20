#pragma once

#include "message.hpp"
#include "tile_info.hpp"
#include "coverage_update_descriptor.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../drape/glstate.hpp"
#include "../drape/pointers.hpp"
#include "../drape/vertex_array_buffer.hpp"

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

  class DropTilesMessage : public Message
  {
  public:
    DropTilesMessage(CoverageUpdateDescriptor const & descr)
      : m_coverageUpdateDescr(descr)
    {
      SetType(DropTiles);
    }

    CoverageUpdateDescriptor const & GetDescriptor() const
    {
      return m_coverageUpdateDescr;
    }

  private:
    CoverageUpdateDescriptor m_coverageUpdateDescr;
  };

  class FlushTileMessage : public BaseTileMessage
  {
  public:
    FlushTileMessage(const TileKey & key, const GLState state, TransferPointer<VertexArrayBuffer> buffer)
      : BaseTileMessage(key, Message::FlushTile)
      , m_state(state)
      , m_buffer(buffer)
    {
    }

    ~FlushTileMessage()
    {
      m_buffer.Destroy();
    }

    const GLState & GetState() const { return m_state; }
    MasterPointer<VertexArrayBuffer> AcceptBuffer() { return MasterPointer<VertexArrayBuffer>(m_buffer); }

  private:
    GLState m_state;
    TransferPointer<VertexArrayBuffer> m_buffer;
  };

  class ResizeMessage : public Message
  {
  public:
    ResizeMessage(int x, int y, int w, int h) : m_rect(x, y, x + w, y + h)
    {
      SetType(Resize);
    }
    ResizeMessage(m2::RectI const & rect) : m_rect(rect)
    {
      SetType(Resize);
    }
    const m2::RectI & GetRect() const { return m_rect; }

  private:
    m2::RectI m_rect;
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

  class RotateMessage: public Message
  {
  public:
    RotateMessage(float dstAngleRadians)
      : m_dstAngle(dstAngleRadians)
    {
      SetType(Rotate);
    }

    float GetDstAngle() const { return m_dstAngle; }

  private:
    float m_dstAngle;
  };
}
