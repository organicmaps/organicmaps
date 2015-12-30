#pragma once

#include "std/noncopyable.hpp"

namespace dp
{

class SupportManager : public noncopyable
{
public:
  static SupportManager & Instance();

  // Initialization must be called only when OpenGL context is created.
  void Init();

  bool IsSamsungGoogleNexus() const;
  bool IsAdreno200Device() const;
  bool IsTegraDevice() const;

private:
  SupportManager() = default;
  ~SupportManager() = default;

  bool m_isSamsungGoogleNexus = false;
  bool m_isAdreno200 = false;
  bool m_isTegra = false;
};

} // namespace dp
