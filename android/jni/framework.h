#pragma once

#include "../../map/framework.hpp"
#include "../../map/feature_vec_model.hpp"
#include "../../map/window_handle.hpp"

#include "../../storage/storage.hpp"

#include <jni.h>


class AndroidFramework
{
public:
  class ViewHandle : public WindowHandle
  {
    AndroidFramework * m_parent;
  public:
    ViewHandle(AndroidFramework * parent) : m_parent(parent) {}
    virtual void invalidateImpl();
  };

private:
  shared_ptr<ViewHandle> m_view;
  Framework<model::FeaturesFetcher> m_work;
  storage::Storage m_storage;

  JNIEnv * m_env;
  jobject m_parentView;

  void CallRepaint();

public:
  AndroidFramework();

  storage::Storage & Storage() { return m_storage; }

  void SetParentView(JNIEnv * env, jobject view);

  void InitRenderer();

  void Resize(int w, int h);

  void DrawFrame();

  void Move(int mode, double x, double y);
  void Zoom(int mode, double x1, double y1, double x2, double y2);
};
