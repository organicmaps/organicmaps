#pragma once

#include "../base/fence_manager.hpp"

#include "../base/threaded_list.hpp"
#include "../base/mutex.hpp"
#include "../base/condition.hpp"

#include "../std/shared_ptr.hpp"

namespace yg
{
  namespace gl
  {
    struct BaseState
    {
      bool m_isDebugging;
      BaseState();
      virtual ~BaseState();
      virtual void apply(BaseState const * prev) = 0;
    };

    struct Command
    {
    private:
      bool m_isDebugging;
    public:

      bool isDebugging() const;
      void setIsDebugging(bool flag);

      Command();

      virtual ~Command();
      virtual void perform() = 0;

      friend class Renderer;
    };

    struct Packet
    {
      shared_ptr<BaseState> m_state;
      shared_ptr<Command> m_command;
      bool m_groupBoundary;

      explicit Packet();
      /// empty packet act as a frame delimiter
      explicit Packet(bool groupBoundary);
      /// non-opengl command, without any state
      explicit Packet(shared_ptr<Command> const & command,
                      bool groupBoundary);
      /// opengl command with state
      explicit Packet(shared_ptr<BaseState> const & state,
                      shared_ptr<Command> const & command,
                      bool groupBoundary);
    };

    class PacketsQueue : public ThreadedList<Packet>
    {
    private:

      FenceManager m_fenceManager;

    public:

      PacketsQueue();

      void markFrameBoundary();
      int  insertFence();
      void joinFence(int id);

      /// Convenience functions

      void completeCommands();
    };
  }
}
