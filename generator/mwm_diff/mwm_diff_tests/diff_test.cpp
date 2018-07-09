#include "testing/testing.hpp"

#include "generator/mwm_diff/diff.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/scope_guard.hpp"

using namespace std;

namespace generator
{
namespace mwm_diff
{
UNIT_TEST(IncrementalUpdates_Smoke)
{
  string const oldMwmPath = my::JoinFoldersToPath(GetPlatform().WritableDir(), "minsk-pass.mwm");
  string const newMwmPath1 =
      my::JoinFoldersToPath(GetPlatform().WritableDir(), "minsk-pass-new1.mwm");
  string const newMwmPath2 =
      my::JoinFoldersToPath(GetPlatform().WritableDir(), "minsk-pass-new2.mwm");
  string const diffPath = my::JoinFoldersToPath(GetPlatform().WritableDir(), "minsk-pass.mwmdiff");

  MY_SCOPE_GUARD(cleanup, [&] {
    FileWriter::DeleteFileX(newMwmPath1);
    FileWriter::DeleteFileX(newMwmPath2);
    FileWriter::DeleteFileX(diffPath);
  });

  {
    // Create an empty file.
    FileWriter writer(newMwmPath1);
  }

  TEST(MakeDiff(oldMwmPath, newMwmPath1, diffPath), ());
  TEST(ApplyDiff(oldMwmPath, newMwmPath2, diffPath), ());

  TEST(my::IsEqualFiles(newMwmPath1, newMwmPath2), ());
}
}  // namespace mwm_diff
}  // namespace generator
