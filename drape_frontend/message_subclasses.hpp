#pragma once

#include "message.hpp"
#include "tile_info.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../drape/glstate.hpp"
#include "../drape/pointers.hpp"

class VertexArrayBuffer;
namespace threads { class IRoutine; }

namespace df
{
  class DropCoverageMessage : public Message
  {
  public:
    DropCoverageMessage() { SetType(DropCoverage); }
  };

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

  class DropTileMessage : public BaseTileMessage
  {
  public:
    DropTileMessage(const TileKey & key)
      : BaseTileMessage(key, Message::DropTile) {}
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

    const GLState & GetState() const { return m_state; }
    TransferPointer<VertexArrayBuffer> & GetBuffer();

  private:
    GLState m_state;
    TransferPointer<VertexArrayBuffer> m_buffer;
  };

  class ResizeMessage : public Message
  {
  public:
    ResizeMessage(int x, int y, int w, int h) : m_rect(x, y, x + w, y + h) {}
    ResizeMessage(m2::RectI const & rect) : m_rect(rect) {}
    const m2::RectI & GetRect() const { return m_rect; }

  private:
    m2::RectI m_rect;
  };

  class TaskFinishMessage : public Message
  {
  public:
    TaskFinishMessage(threads::IRoutine * routine) : m_routine(routine) {}
    threads::IRoutine * GetRoutine() const { return m_routine; }

  private:
    threads::IRoutine * m_routine;
  };

  class UpdateCoverageMessage : public Message
  {
  public:
    UpdateCoverageMessage(ScreenBase const & screen) : m_screen(screen) {}
    const ScreenBase & GetScreen() const { return m_screen; }

  private:
    ScreenBase m_screen;
  };
}
