#include "testing/testing.hpp"

#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/atomic_shared_ptr.hpp"


namespace
{
using namespace editor;
using platform::tests_support::ScopedFile;

void CheckGeneralTags(std::string const & jsonContent)
{
  TEST(!jsonContent.empty(), ("JSON content is empty"));
  TEST(jsonContent.find("\"fields\"") != std::string::npos, ("'fields' key is missing"));
  TEST(jsonContent.find("\"types\"") != std::string::npos, ("'types' key is missing"));
}

UNIT_TEST(ConfigLoader_Base)
{
  base::AtomicSharedPtr<EditorConfig> config;
  ConfigLoader loader(config);

  TEST(!config.Get()->GetTypesThatCanBeAdded().empty(), ());
}

// This functionality is not used and corresponding server is not working.
// Uncomment it when server will be up.
// UNIT_TEST(ConfigLoader_GetRemoteHash)
//{
//  auto const hashStr = ConfigLoader::GetRemoteHash();
//  TEST_NOT_EQUAL(hashStr, "", ());
//  TEST_EQUAL(hashStr, ConfigLoader::GetRemoteHash(), ());
//}
//
// UNIT_TEST(ConfigLoader_GetRemoteConfig)
//{
//  pugi::xml_document doc;
//  ConfigLoader::GetRemoteConfig(doc);
//  CheckGeneralTags(doc);
//}

UNIT_TEST(ConfigLoader_LoadFromLocal)
{
  auto const content = ConfigLoader::LoadFromLocal();
  CheckGeneralTags(content);
}
}  // namespace