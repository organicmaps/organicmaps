#include "testing/testing.hpp"

#include "platform/platform.hpp"
#include "platform/platform_linux_migration.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "base/file_name_utils.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>

namespace platform_linux_migration_test
{
namespace fs = std::filesystem;

std::string TestRoot(std::string const & name)
{
  return base::JoinPath(GetPlatform().WritableDir(), name + std::to_string(::getpid()));
}
UNIT_TEST(MigrateDesktopDirectory_FirstLaunch)
{
  auto const root = TestRoot("MigrateDesktopDirectoryFirstLaunch");
  platform::tests_support::ScopedDirCleanup const cleanup(root);
  auto const oldPath = fs::path(root) / "OMaps";
  auto const newPath = fs::path(root) / "OrganicMaps";
  fs::create_directories(oldPath / "bookmarks");
  std::ofstream(oldPath / "bookmarks" / "saved.kmz") << "saved";

  TEST_EQUAL(platform::MigrateDesktopDirectory(root, true), newPath.string(), ());
  TEST(!fs::exists(oldPath), ());
  TEST(fs::exists(newPath / "bookmarks" / "saved.kmz"), ());
  TEST_EQUAL(platform::MigrateDesktopDirectory(root, true), newPath.string(), ());
}

UNIT_TEST(MigrateDesktopDirectory_ExistingDestination)
{
  auto const root = TestRoot("MigrateDesktopDirectoryExistingDestination");
  platform::tests_support::ScopedDirCleanup const cleanup(root);
  auto const oldPath = fs::path(root) / "OMaps";
  auto const newPath = fs::path(root) / "OrganicMaps";
  fs::create_directories(oldPath);
  fs::create_directories(newPath);
  std::ofstream(oldPath / "old.kmz") << "old";
  std::ofstream(newPath / "new.kmz") << "new";

  TEST_EQUAL(platform::MigrateDesktopDirectory(root, true), newPath.string(), ());
  TEST(fs::exists(oldPath / "old.kmz"), ());
  TEST(fs::exists(newPath / "new.kmz"), ());
}

UNIT_TEST(MigrateDesktopDirectory_EmptyDestination)
{
  auto const root = TestRoot("MigrateDesktopDirectoryEmptyDestination");
  platform::tests_support::ScopedDirCleanup const cleanup(root);
  auto const oldPath = fs::path(root) / "OMaps";
  auto const newPath = fs::path(root) / "OrganicMaps";
  fs::create_directories(oldPath);
  fs::create_directories(newPath);
  std::ofstream(oldPath / "old.kmz") << "old";

  TEST_EQUAL(platform::MigrateDesktopDirectory(root, true), newPath.string(), ());
  TEST(!fs::exists(oldPath), ());
  TEST(fs::exists(newPath / "old.kmz"), ());
}

UNIT_TEST(MigrateDesktopDirectory_Symlinks)
{
  auto const root = TestRoot("MigrateDesktopDirectorySymlinks");
  platform::tests_support::ScopedDirCleanup const cleanup(root);
  auto const oldPath = fs::path(root) / "OMaps";
  auto const newPath = fs::path(root) / "OrganicMaps";
  auto const oldTarget = fs::path(root) / "old-target";
  auto const newTarget = fs::path(root) / "new-target";
  fs::create_directories(oldTarget);
  fs::create_directories(newTarget);
  fs::create_directory_symlink(oldTarget, oldPath);

  TEST_EQUAL(platform::MigrateDesktopDirectory(root, true), oldPath.string(), ());
  TEST(fs::is_symlink(oldPath), ());

  fs::create_directory_symlink(newTarget, newPath);
  TEST_EQUAL(platform::MigrateDesktopDirectory(root, true), newPath.string(), ());
  TEST(fs::is_symlink(oldPath), ());
  TEST(fs::is_symlink(newPath), ());
  fs::remove(oldPath);
  fs::create_directories(oldPath);
  std::ofstream(oldPath / "old.kmz") << "old";
  TEST_EQUAL(platform::MigrateDesktopDirectory(root, true), oldPath.string(), ());
  TEST(fs::exists(oldPath / "old.kmz"), ());
  TEST(fs::is_symlink(newPath), ());
  fs::remove(newPath);
}

UNIT_TEST(MigrateDesktopDirectory_FreshInstall)
{
  auto const root = TestRoot("MigrateDesktopDirectoryFreshInstall");
  platform::tests_support::ScopedDirCleanup const cleanup(root);
  auto const newPath = fs::path(root) / "OrganicMaps";

  TEST_EQUAL(platform::MigrateDesktopDirectory(root, true), newPath.string(), ());
  TEST(!fs::exists(newPath), ());
}

UNIT_TEST(MigrateDesktopDirectory_OtherTool)
{
  auto const root = TestRoot("MigrateDesktopDirectoryOtherTool");
  platform::tests_support::ScopedDirCleanup const cleanup(root);
  auto const oldPath = fs::path(root) / "OMaps";
  auto const newPath = fs::path(root) / "OrganicMaps";

  TEST_EQUAL(platform::MigrateDesktopDirectory(root, false), oldPath.string(), ());
  fs::create_directories(oldPath);
  TEST_EQUAL(platform::MigrateDesktopDirectory(root, false), oldPath.string(), ());
  TEST_EQUAL(platform::MigrateDesktopDirectory(root, true), newPath.string(), ());
  TEST_EQUAL(platform::MigrateDesktopDirectory(root, false), newPath.string(), ());
}
}  // namespace platform_linux_migration_test
