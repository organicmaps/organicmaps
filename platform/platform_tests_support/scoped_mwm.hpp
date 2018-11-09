#pragma once

#include "scoped_file.hpp"

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
  ScopedMwm(string const & relativePath);

  string const & GetFullPath() const { return m_file.GetFullPath(); }

private:
  ScopedFile m_file;

  DISALLOW_COPY_AND_MOVE(ScopedMwm);
};
}  // namespace tests_support
}  // namespace platform
