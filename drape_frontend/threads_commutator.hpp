#pragma once

#include "../std/map.hpp"

namespace df
{
  class Message;
  class MessageAcceptor;

  class ThreadsCommutator
  {
  public:
    enum ThreadName
    {
      RenderThread,
      ResourceUploadThread
    };

    void RegisterThread(ThreadName name, MessageAcceptor * acceptor);
    void PostMessage(ThreadName name, Message * message);

  private:
    typedef map<ThreadName, MessageAcceptor *> acceptors_map_t;
     acceptors_map_t m_acceptors;
  };
}
