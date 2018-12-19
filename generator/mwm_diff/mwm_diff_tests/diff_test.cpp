#include "testing/testing.hpp"

#include "generator/mwm_diff/diff.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

namespace generator
{
namespace mwm_diff
{
UNIT_TEST(IncrementalUpdates_Smoke)
{
  base::ScopedLogAbortLevelChanger ignoreLogError(base::LogLevel::LCRITICAL);

  string const oldMwmPath = base::JoinFoldersToPath(GetPlatform().WritableDir(), "minsk-pass.mwm");
  string const newMwmPath1 =
      base::JoinFoldersToPath(GetPlatform().WritableDir(), "minsk-pass-new1.mwm");
  string const newMwmPath2 =
      base::JoinFoldersToPath(GetPlatform().WritableDir(), "minsk-pass-new2.mwm");
  string const diffPath = base::JoinFoldersToPath(GetPlatform().WritableDir(), "minsk-pass.mwmdiff");

  SCOPE_GUARD(cleanup, [&] {
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

  {
    // Alter the old mwm slightly.
    vector<uint8_t> oldMwmContents = FileReader(oldMwmPath).ReadAsBytes();
    size_t const sz = oldMwmContents.size();
    for (size_t i = 3 * sz / 10; i < 4 * sz / 10; i++)
      oldMwmContents[i] += static_cast<uint8_t>(i);

    FileWriter writer(newMwmPath1);
    writer.Write(oldMwmContents.data(), oldMwmContents.size());
  }

  TEST(MakeDiff(oldMwmPath, newMwmPath1, diffPath), ());
  TEST(ApplyDiff(oldMwmPath, newMwmPath2, diffPath), ());

  TEST(base::IsEqualFiles(newMwmPath1, newMwmPath2), ());

  {
    // Corrupt the diff file contents.
    vector<uint8_t> diffContents = FileReader(diffPath).ReadAsBytes();

    // Leave the version bits intact.
    for (size_t i = 4; i < diffContents.size(); i += 2)
      diffContents[i] ^= 255;

    FileWriter writer(diffPath);
    writer.Write(diffContents.data(), diffContents.size());
  }

  TEST(!ApplyDiff(oldMwmPath, newMwmPath2, diffPath), ());

  {
    // Reset the diff file contents.
    FileWriter writer(diffPath);
  }

  TEST(!ApplyDiff(oldMwmPath, newMwmPath2, diffPath), ());
}
}  // namespace mwm_diff
}  // namespace generator
