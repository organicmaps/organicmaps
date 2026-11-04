#include "testing/testing.hpp"

#include "editor/changeset_wrapper.hpp"

namespace changeset_wrapper_test
{
// Check that repeated Finish() calls and destruction without an opened changeset do not crash.
UNIT_TEST(ChangesetWrapper_FinishWithoutChangesetIsIdempotent)
{
  osm::ChangesetWrapper changeset("" /* keySecret */, {{"created_by", "Organic Maps unit test"}});
  changeset.Finish();
  changeset.Finish();
  // The destructor calls Finish() once more.
}
}  // namespace changeset_wrapper_test
