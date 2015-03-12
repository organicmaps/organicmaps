#pragma once

#include "drape/pointers.hpp"
#include "std/map.hpp"

namespace df
{

class Message;
enum class MessagePriority;
class MessageAcceptor;

class ThreadsCommutator
{
public:
  enum ThreadName
  {
    RenderThread,
    ResourceUploadThread
  };

  void RegisterThread(ThreadName name, MessageAcceptor *acceptor);
  void PostMessage(ThreadName name, dp::TransferPointer<Message> message, MessagePriority priority);
  void PostMessageBroadcast(dp::TransferPointer<Message> message, MessagePriority priority);

private:
  typedef map<ThreadName, MessageAcceptor *> acceptors_map_t;
  acceptors_map_t m_acceptors;
};

} // namespace df
