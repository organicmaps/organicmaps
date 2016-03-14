#include "search/search_quality/helpers.hpp"

#include "std/target_os.hpp"

#include <sys/resource.h>

namespace search
{
void ChangeMaxNumberOfOpenFiles(size_t n)
{
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_LINUX)
  struct rlimit rlp;
  getrlimit(RLIMIT_NOFILE, &rlp);
  rlp.rlim_cur = n;
  setrlimit(RLIMIT_NOFILE, &rlp);
#endif
}
}  // namespace search
