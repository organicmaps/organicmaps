#pragma once

#include "../../map/framework.hpp"
#include "../../map/drawer_yg.hpp"
#include "../../map/window_handle.hpp"
#include "../../map/feature_vec_model.hpp"

#include "../../storage/storage.hpp"

#include <jni.h>

class AndroidFramework
{

private:
  shared_ptr<DrawerYG> m_drawer;
  shared_ptr<WindowHandle> m_handle;

  Framework<model::FeaturesFetcher> m_work;
  storage::Storage m_storage;

  JavaVM * m_jvm;
  jobject m_parentView;

  void CallRepaint();

public:
  AndroidFramework(JavaVM * jvm);

  storage::Storage & Storage() { return m_storage; }

  void SetParentView(jobject view);

  void InitRenderer();

  void Resize(int w, int h);

  void DrawFrame();

  void Move(int mode, double x, double y);
  void Zoom(int mode, double x1, double y1, double x2, double y2);

  void EnableLocation(bool enable);
  void UpdateLocation(uint64_t timestamp, double lat, double lon, float accuracy);
  void UpdateCompass(uint64_t timestamp, double magneticNorth, double trueNorth, float accuracy);
};
