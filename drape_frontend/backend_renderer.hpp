#pragma once

#include "../geometry/screenbase.hpp"

#include "../std/function.hpp"

namespace df
{
  class ThreadsCommutator;
  class BackendRendererImpl;
  class Message;

  typedef function<void (Message *)> post_message_fn_t;

  class BackendRenderer
  {
  public:
    BackendRenderer(ThreadsCommutator * commutator,
                    double visualScale,
                    int surfaceWidth,
                    int surfaceHeight);

    ~BackendRenderer();

    void UpdateCoverage(const ScreenBase & screen);
    void Resize(int x0, int y0, int w, int h);

  private:
    BackendRendererImpl * m_impl;
    post_message_fn_t m_post;
  };
}
