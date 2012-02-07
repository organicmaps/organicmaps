#pragma once

#include <jni.h>

#include "../../../../../map/framework.hpp"
#include "../../../../../map/drawer_yg.hpp"
#include "../../../../../map/window_handle.hpp"
#include "../../../../../map/feature_vec_model.hpp"
#include "../../../../../base/timer.hpp"
#include "../../../nv_event/nv_event.hpp"

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

    NVMultiTouchEventType m_eventType; //< multitouch action

    double m_x1;
    double m_y1;
    double m_x2;
    double m_y2;

    bool m_hasFirst;
    bool m_hasSecond;
    int m_mask;

    /// single click processing parameters

    my::Timer m_doubleClickTimer;
    bool m_isInsideDoubleClick;
    bool m_isCleanSingleClick;
    double m_lastX1;
    double m_lastY1;

  public:

    Framework(JavaVM * jvm);
    ~Framework();

    void SetEmptyModelMessage(jstring emptyModelMsg);

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
    void Touch(int action, int mask, double x1, double y1, double x2, double y2);

    void LoadState();
    void SaveState();

    void SetupMeasurementSystem();

    void AddLocalMaps() { m_work.AddLocalMaps(); }
    void RemoveLocalMaps() { m_work.RemoveLocalMaps(); }
  };
}

extern android::Framework * g_framework;
