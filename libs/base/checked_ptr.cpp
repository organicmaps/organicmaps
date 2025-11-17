#include "base/checked_ptr.hpp"

namespace check_impl
{
void not_null(void const * p)
{
  CHECK(p, ());
}

void equal_thread(std::thread::id id)
{
  CHECK_EQUAL(id, std::this_thread::get_id(), ());
}
}  // namespace check_impl
