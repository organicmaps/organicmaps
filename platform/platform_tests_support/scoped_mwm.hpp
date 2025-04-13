#pragma once

#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/macros.hpp"

#include <string>

namespace platform
{
namespace tests_support
{
class ScopedFile;

class ScopedMwm
{
public:
  explicit ScopedMwm(std::string const & relativePath);

  std::string const & GetFullPath() const { return m_file.GetFullPath(); }

private:
  ScopedFile m_file;

  DISALLOW_COPY_AND_MOVE(ScopedMwm);
};
}  // namespace tests_support
}  // namespace platform
