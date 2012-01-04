#include "packets_queue.hpp"
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    BaseState::BaseState()
      : m_isDebugging(false)
    {}

    BaseState::~BaseState()
    {}

    bool Command::isDebugging() const
    {
      return m_isDebugging;
    }

    void Command::setIsDebugging(bool flag)
    {
      m_isDebugging = flag;
    }

    Command::Command()
      : m_isDebugging(false)
    {}

    Command::~Command()
    {}

    void Command::cancel()
    {
      if ((m_isDebugging) && (!m_name.empty()))
        LOG(LINFO, ("cancelling", m_name, "command"));
    }

    void Command::perform()
    {
      if ((m_isDebugging) && (!m_name.empty()))
        LOG(LINFO, ("performing", m_name, "command"));
    }

    Packet::Packet()
    {}

    Packet::Packet(EType type)
      : m_type(type)
    {}

    Packet::Packet(shared_ptr<Command> const & command, EType type)
      : m_command(command),
        m_type(type)
    {}

    Packet::Packet(shared_ptr<BaseState> const & state,
                   shared_ptr<Command> const & command,
                   EType type)
      : m_state(state),
        m_command(command),
        m_type(type)
    {
      if (m_state && m_command)
        m_state->m_isDebugging = m_command->isDebugging();
    }

    PacketsQueue::PacketsQueue() : m_fenceManager(5)
    {}

    void PacketsQueue::insertCheckPoint()
    {
      processPacket(Packet(Packet::ECheckPoint));
    }

    void PacketsQueue::insertCancelPoint()
    {
      processPacket(Packet(Packet::ECancelPoint));
    }

    struct SignalFence : public Command
    {
      int m_id;
      FenceManager * m_fenceManager;

      SignalFence(int id, FenceManager * fenceManager)
        : m_id(id), m_fenceManager(fenceManager)
      {}

      void perform()
      {
        m_fenceManager->signalFence(m_id);
      }

      void cancel()
      {
        perform();
      }
    };

    int PacketsQueue::insertFence()
    {
      int id = m_fenceManager.insertFence();
      processPacket(Packet(make_shared_ptr(new SignalFence(id, &m_fenceManager)), Packet::ECheckPoint));
      return id;
    }

    void PacketsQueue::joinFence(int id)
    {
      m_fenceManager.joinFence(id);
    }

    void PacketsQueue::completeCommands()
    {
      joinFence(insertFence());
    }

    void PacketsQueue::cancel()
    {
      Cancel();
    }

    void PacketsQueue::processPacket(Packet const & packet)
    {
      if (IsCancelled())
      {
        if (packet.m_command)
          packet.m_command->cancel();
      }
      else
        PushBack(packet);
    }
  }
}
