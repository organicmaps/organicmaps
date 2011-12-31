#include "packets_queue.hpp"

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

    Packet::Packet()
      : m_groupBoundary(true)
    {}

    Packet::Packet(bool groupBoundary)
      : m_groupBoundary(groupBoundary)
    {}

    Packet::Packet(shared_ptr<Command> const & command,
                   bool groupBoundary)
      : m_command(command),
        m_groupBoundary(groupBoundary)
    {}

    Packet::Packet(shared_ptr<BaseState> const & state,
                   shared_ptr<Command> const & command,
                   bool groupBoundary)
      : m_state(state),
        m_command(command),
        m_groupBoundary(groupBoundary)
    {
      if (m_state && m_command)
        m_state->m_isDebugging = m_command->isDebugging();
    }

    PacketsQueue::PacketsQueue() : m_fenceManager(10)
    {}

    void PacketsQueue::markFrameBoundary()
    {
      PushBack(Packet(true));
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
    };

    int PacketsQueue::insertFence()
    {
      int id = m_fenceManager.insertFence();
      PushBack(Packet(make_shared_ptr(new SignalFence(id, &m_fenceManager)), true));
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
  }
}
