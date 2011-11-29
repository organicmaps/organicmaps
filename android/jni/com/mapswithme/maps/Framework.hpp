/*
 * Framework.hpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */
#pragma once

#include <jni.h>

#include "../../../../../map/framework.hpp"
#include "../../../../../map/drawer_yg.hpp"
#include "../../../../../map/window_handle.hpp"
#include "../../../../../map/feature_vec_model.hpp"
#include "../../../nv_event/nv_event.hpp"
#include "../../../../../platform/location_service.hpp"

namespace android
{
  class Framework : public location::LocationObserver
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

  public:

    Framework(JavaVM * jvm);
    ~Framework();

    storage::Storage & Storage();

    void OnLocationStatusChanged(location::TLocationStatus newStatus);
    void OnGpsUpdated(location::GpsInfo const & info);

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

    void EnableLocation(bool enable);
    void UpdateLocation(uint64_t timestamp, double lat, double lon, float accuracy);
    void UpdateCompass(uint64_t timestamp, double magneticNorth, double trueNorth, float accuracy);
  };
}

extern android::Framework * g_framework;
