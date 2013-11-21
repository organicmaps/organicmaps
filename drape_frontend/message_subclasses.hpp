#pragma once

#include "message.hpp"
#include "tile_info.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

namespace threads { class IRoutine; }

namespace df
{
  class DropCoverageMessage : public Message
  {
  public:
    DropCoverageMessage() { SetType(DropCoverage); }
  };

  class DropTileMessage : public Message
  {
  public:
    DropTileMessage(const TileKey & tileKey) : m_tileKey(tileKey) {}
    const TileKey & GetKey() const { return m_tileKey; }

  private:
    TileKey m_tileKey;
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
