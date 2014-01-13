#pragma once

#include "../drape/pointers.hpp"
#include "../drape/oglcontextfactory.hpp"

#include "../geometry/screenbase.hpp"

#include "../std/function.hpp"

namespace df
{
  class ThreadsCommutator;
  class BackendRendererImpl;
  class Message;

  class BackendRenderer
  {
  public:
    BackendRenderer(RefPointer<ThreadsCommutator> commutator,
                    RefPointer<OGLContextFactory> oglcontextfactory,
                    double visualScale,
                    int surfaceWidth,
                    int surfaceHeight);

    ~BackendRenderer();

    void UpdateCoverage(const ScreenBase & screen);
    void Resize(int x0, int y0, int w, int h);

  private:
    MasterPointer<BackendRendererImpl> m_impl;
    RefPointer<ThreadsCommutator> m_commutator;
  };
}
