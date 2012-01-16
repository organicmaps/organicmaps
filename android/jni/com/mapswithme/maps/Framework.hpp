#pragma once

#include <jni.h>

#include "../../../../../map/framework.hpp"
#include "VideoTimer.hpp"

namespace android
{
  class Framework
  {
  private:
    ::Framework m_work;

    android::VideoTimer m_videoTimer;

    void CallRepaint();

    void CreateDrawer();

    void CreateResourceManager();

  public:

    Framework();
    ~Framework();

    storage::Storage & Storage();

    void OnLocationStatusChanged(int/* == location::TLocationStatus*/ newStatus);
    void OnLocationUpdated(uint64_t time, double lat, double lon, float accuracy);
    void OnCompassUpdated(uint64_t time, double magneticNorth, double trueNorth, float accuracy);

    void Invalidate();

    bool InitRenderPolicy();
    void DeleteRenderPolicy();

    void Resize(int w, int h);

    void DrawFrame();

    void Move(int mode, double x, double y);
    void Zoom(int mode, double x1, double y1, double x2, double y2);

    void LoadState();
    void SaveState();

    void SetupMeasurementSystem();

    void AddLocalMaps() { m_work.AddLocalMaps(); }
    void RemoveLocalMaps() { m_work.RemoveLocalMaps(); }
  };

  Framework & GetFramework();
}
