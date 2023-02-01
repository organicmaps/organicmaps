#include "testing/testing.hpp"

#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/atomic_shared_ptr.hpp"

#include <pugixml.hpp>

namespace
{
using namespace editor;
using platform::tests_support::ScopedFile;

void CheckGeneralTags(pugi::xml_document const & doc)
{
  auto const types = doc.select_nodes("/omaps/editor/types");
  TEST(!types.empty(), ());
  auto const fields = doc.select_nodes("/omaps/editor/fields");
  TEST(!fields.empty(), ());
  auto const preferred_types = doc.select_nodes("/omaps/editor/preferred_types");
  TEST(!preferred_types.empty(), ());
}

UNIT_TEST(ConfigLoader_Base)
{
  base::AtomicSharedPtr<EditorConfig> config;
  ConfigLoader loader(config);

  TEST(!config.Get()->GetTypesThatCanBeAdded().empty(), ());
}

// This functionality is not used and corresponding server is not working.
// Uncomment it when server will be up.
//UNIT_TEST(ConfigLoader_GetRemoteHash)
//{
//  auto const hashStr = ConfigLoader::GetRemoteHash();
//  TEST_NOT_EQUAL(hashStr, "", ());
//  TEST_EQUAL(hashStr, ConfigLoader::GetRemoteHash(), ());
//}
//
//UNIT_TEST(ConfigLoader_GetRemoteConfig)
//{
//  pugi::xml_document doc;
//  ConfigLoader::GetRemoteConfig(doc);
//  CheckGeneralTags(doc);
//}

UNIT_TEST(ConfigLoader_SaveLoadHash)
{
  ScopedFile sf("test.hash", ScopedFile::Mode::Create);
  auto const testHash = "12345 678909 87654 321 \n 32";

  ConfigLoader::SaveHash(testHash, sf.GetFullPath());
  auto const loadedHash = ConfigLoader::LoadHash(sf.GetFullPath());
  TEST_EQUAL(testHash, loadedHash, ());
}

UNIT_TEST(ConfigLoader_LoadFromLocal)
{
  pugi::xml_document doc;
  ConfigLoader::LoadFromLocal(doc);
  CheckGeneralTags(doc);
}
}  // namespace
