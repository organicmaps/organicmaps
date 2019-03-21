#pragma once

#include "map/framework.hpp"
#include "map/framework_light.hpp"

// Implemented in assume that delegate lifetime is shorter than Framework lifetime.
// In other words lightweight::Framework lifetime should be shorter than Framework lifetime.
class FrameworkLightDelegate : public lightweight::Framework::Delegate
{
public:
  explicit FrameworkLightDelegate(Framework & framework) : m_framework(framework) {}

  notifications::NotificationManager & GetNotificationManager() override
  {
    return m_framework.GetNotificationManager();
  }

private:
  Framework & m_framework;
};
