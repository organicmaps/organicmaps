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

    template <typename Fn>
    struct FunctorCommand : Command
    {
      Fn m_fn;
      bool m_performOnCancel;

      FunctorCommand(Fn fn, bool performOnCancel = false)
        : m_fn(fn), m_performOnCancel(performOnCancel)
      {}

      void perform()
      {
        m_fn();
      }

      void cancel()
      {
        if (m_performOnCancel)
          m_fn();
      }
    };

    struct Packet
    {
      enum EType
      {
        ECommand,
        ECheckPoint,
        ECancelPoint
      };

      shared_ptr<Command>   m_command;
      EType m_type;

      Packet();
      /// empty packet act as a frame delimiter
      explicit Packet(EType type);
      /// simple command
      Packet(shared_ptr<Command> const & command,
             EType type);
    };

    class PacketsQueue
    {
    private:

      ThreadedList<Packet> m_packets;
      FenceManager m_fenceManager;

    public:

      PacketsQueue();

      void processPacket(Packet const & packet);
      void cancel();
      bool empty() const;
      size_t size() const;

      int  insertFence(Packet::EType type);
      void joinFence(int id);

      /// Convenience functions

      void completeCommands();
      void cancelCommands();

      template <typename Fn>
      void processFn(Fn fn, bool performOnCancel = false)
      {
        processPacket(Packet(make_shared_ptr(new FunctorCommand<Fn>(fn)), Packet::ECommand));
      }

      template <typename Fn>
      void processList(Fn fn)
      {
        m_packets.ProcessList(fn);
      }
    };
  }
}
