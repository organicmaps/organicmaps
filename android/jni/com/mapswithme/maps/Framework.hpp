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

namespace android
{
  class Framework
  {
  private:

    JavaVM * m_jvm;
    jobject m_mainGLView;

    ::Framework m_work;

    VideoTimer * m_videoTimer;

    void CallRepaint();

    void CreateDrawer();

    void CreateResourceManager();

  public:

    Framework(JavaVM * jvm);
    ~Framework();

    storage::Storage & Storage();

    void SetParentView(jobject view);

    void InitRenderPolicy();

    void Resize(int w, int h);

    void DrawFrame();

    void Move(int mode, double x, double y);
    void Zoom(int mode, double x1, double y1, double x2, double y2);

    void EnableLocation(bool enable);
    void UpdateLocation(uint64_t timestamp, double lat, double lon, float accuracy);
    void UpdateCompass(uint64_t timestamp, double magneticNorth, double trueNorth, float accuracy);

    JavaVM * javaVM() const;
  };
}

extern android::Framework * g_framework;
