#pragma once

#include "drape/pointers.hpp"

#include <atomic>
#include <map>

namespace df
{

class Message;
enum class MessagePriority;
class BaseRenderer;

class ThreadsCommutator
{
public:
  enum ThreadName
  {
    RenderThread,
    ResourceUploadThread
  };

  std::atomic<unsigned> m_contextCounter{0};

  void RegisterThread(ThreadName name, BaseRenderer * acceptor);
  void PostMessage(ThreadName name, drape_ptr<Message> && message, MessagePriority priority);

private:
  using TAcceptorsMap = std::map<ThreadName, BaseRenderer *>;
  TAcceptorsMap m_acceptors;
};

}  // namespace df
