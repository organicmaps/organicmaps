#pragma once

namespace yg
{
  namespace gl
  {
    /// Synchronization mechanism
    class Fence
    {
    private:
      unsigned int m_id;
      bool m_inserted;
    public:
      Fence();
      ~Fence();
      /// insert fence in the opengl command stream
      void insert();
      /// wait until signaled
      void wait();
      /// get the completion status
      /// true - fence reached
      /// false - not reached
      bool status();
    };
  }
}
