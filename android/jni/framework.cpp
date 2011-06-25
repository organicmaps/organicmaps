#include "framework.h"
#include "jni_helper.h"

#include "../../map/drawer_yg.hpp"


void AndroidFramework::ViewHandle::invalidateImpl()
{
  m_parent->CallRepaint();
}

void AndroidFramework::CallRepaint()
{
  m_env->CallVoidMethod(m_parentView,
          jni::GetJavaMethodID(m_env, m_parentView, "requestRender", "()V"));
}

AndroidFramework::AndroidFramework()
: m_view(new ViewHandle(this)), m_work(m_view, 0)
{
  m_work.InitStorage(m_storage);
}

void AndroidFramework::SetParentView(JNIEnv * env, jobject view)
{
  m_env = env;
  m_parentView = view;
}

void AndroidFramework::Init()
{
  //initializeGL();
  //m_view->setDrawer(new DrawerYG());
  //m_view->setRenderContext();
}
