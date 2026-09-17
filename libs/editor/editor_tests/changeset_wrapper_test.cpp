#include "testing/testing.hpp"

#include "editor/changeset_wrapper.hpp"

namespace changeset_wrapper_test
{
// A wrapper that never opened a changeset must not contact the server, so Finish() is a no-op and
// the destructor may call it again. Any regression here would hang or fail this test on the network.
UNIT_TEST(ChangesetWrapper_FinishWithoutChangesetIsIdempotent)
{
  osm::ChangesetWrapper changeset("" /* keySecret */, {{"created_by", "Organic Maps unit test"}});
  changeset.Finish();
  changeset.Finish();
  // The destructor calls Finish() once more.
}
}  // namespace changeset_wrapper_test
