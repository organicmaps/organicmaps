#pragma once

#include "platform/platform.hpp"

#include "base/macros.hpp"

#include "std/string.hpp"

namespace platform
{
namespace tests_support
{
class ScopedFile;

class ScopedMwm
{
public:
  ScopedMwm(string const & fullPath);

  inline string const & GetFullPath() const { return m_fullPath; }

  inline void Reset() { m_reset = true; }

  inline bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

  ~ScopedMwm();

private:
  string const m_fullPath;
  bool m_reset;

  DISALLOW_COPY_AND_MOVE(ScopedMwm);
};
}  // namespace tests_support
}  // namespace platform
