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
      string m_name;

    public:

      bool isDebugging() const;
      void setIsDebugging(bool flag);

      Command();

      virtual ~Command();
      virtual void perform();
      virtual void cancel();

      friend class Renderer;
    };

    struct Packet
    {
      enum EType
      {
        ECommand,
        ECheckPoint,
        ECancelPoint
      };

      shared_ptr<BaseState> m_state;
      shared_ptr<Command>   m_command;
      EType m_type;

      Packet();
      /// empty packet act as a frame delimiter
      explicit Packet(EType type);
      /// non-opengl command, without any state
      Packet(shared_ptr<Command> const & command,
             EType type);
      /// opengl command with state
      Packet(shared_ptr<BaseState> const & state,
             shared_ptr<Command> const & command,
             EType type);

    };

    class PacketsQueue : public ThreadedList<Packet>
    {
    private:

      FenceManager m_fenceManager;

    public:

      PacketsQueue();

      void processPacket(Packet const & packet);
      void cancel();

      void insertCheckPoint();
      void insertCancelPoint();
      int  insertFence();
      void joinFence(int id);

      /// Convenience functions

      void completeCommands();
    };
  }
}
