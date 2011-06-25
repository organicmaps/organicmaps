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
  FrameWork<model::FeaturesFetcher> m_work;
  storage::Storage m_storage;

  JNIEnv * m_env;
  jobject m_parentView;

  void CallRepaint();

public:
  AndroidFramework();

  void SetParentView(JNIEnv * env, jobject view);

  void Init();
};
