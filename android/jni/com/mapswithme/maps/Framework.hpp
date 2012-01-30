#pragma once

#include <jni.h>

#include "../../../../../map/framework.hpp"

namespace android
{
  class Framework
  {
  private:
    ::Framework m_work;

    VideoTimer * m_videoTimer;

    void CallRepaint();

    void CreateDrawer();

    void CreateResourceManager();

    double m_x1;
    double m_y1;
    double m_x2;
    double m_y2;

    bool m_hasFirst;
    bool m_hasSecond;
    int m_mask;

  public:

    Framework(JavaVM * jvm);
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
}

extern android::Framework * g_framework;
