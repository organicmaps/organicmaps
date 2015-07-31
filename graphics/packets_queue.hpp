#pragma once

#include "graphics/uniforms_holder.hpp"

#include "base/fence_manager.hpp"

#include "base/threaded_list.hpp"
#include "base/mutex.hpp"
#include "base/condition.hpp"

#include "std/shared_ptr.hpp"

namespace graphics
{
  class RenderContext;

  struct Command
  {
  private:

    bool m_isDebugging;
    string m_name;

    RenderContext * m_ctx;

  public:

    bool isDebugging() const;
    void setIsDebugging(bool flag);

    virtual bool isNeedAdditionalUniforms() const { return false; }
    virtual void setAdditionalUniforms(UniformsHolder const & /*uniforms*/) {}
    virtual void resetAdditionalUniforms() {}

    virtual bool isNeedIndicesCount() const { return false; }
    virtual void setIndicesCount(size_t indicesCount) {}

    void setRenderContext(RenderContext * ctx);
    RenderContext * renderContext();
    RenderContext const * renderContext() const;

    Command();

    virtual ~Command();
    virtual void perform();
    virtual void cancel();
    virtual void dump();
  };

  struct DumpCommand : Command
  {
    shared_ptr<Command> m_cmd;

    DumpCommand(shared_ptr<Command> const & cmd);

    void perform();
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
      EFramePoint,
      ECancelPoint
    };

    shared_ptr<Command>   m_command;
    EType m_type;

    Packet();
    /// empty packet act as a frame delimiter or a checkpoint.
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
    void cancelFences();
    bool empty() const;
    size_t size() const;

    int  insertFence(Packet::EType type);
    void joinFence(int id);

    /// convenience functions

    void completeCommands();
    void cancelCommands();

    template <typename Fn>
    void processFn(Fn const & fn, bool performOnCancel = false)
    {
      processPacket(Packet(make_shared<FunctorCommand<Fn>>(fn), Packet::ECommand));
    }

    template <typename Fn>
    void processList(Fn const & fn)
    {
      m_packets.ProcessList(fn);
    }
  };
}
