#pragma once

#include "platform.hpp"

class CustomIOSPlatform : public Platform
{
public:
  CustomIOSPlatform();

  void MigrateWritableDirForAppleWatch();
};