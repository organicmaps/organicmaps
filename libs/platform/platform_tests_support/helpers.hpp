#pragma once

#if defined(OMIM_OS_MAC) || defined(OMIM_OS_LINUX)
#include <sys/resource.h>
#endif

namespace platform
{
namespace tests_support
{
inline void ChangeMaxNumberOfOpenFiles(size_t n)
{
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_LINUX)
  struct rlimit rlp;
  getrlimit(RLIMIT_NOFILE, &rlp);
  rlp.rlim_cur = n;
  setrlimit(RLIMIT_NOFILE, &rlp);
#endif
}
}  // namespace tests_support
}  // namespace platform
