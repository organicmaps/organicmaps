#include <3party/jansson/jansson/src/jansson.h>
#include "testing/testing.hpp"

#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/atomic_shared_ptr.hpp"

#include "cppjansson/cppjansson.hpp"

namespace
{
using namespace editor;
using platform::tests_support::ScopedFile;

void CheckGeneralTags(base::Json const & doc)
{
  auto const * root = doc.get();
  TEST(root, ("JSON root is null"));
  TEST(json_is_object(root), ("JSON root is not an object"));
  TEST(json_object_get(root, "types"), ("'types' key is missing"));
  TEST(json_object_get(root, "fields"), ("'fields' key is missing"));
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
  base::Json doc;
  ConfigLoader::LoadFromLocal(doc);
  CheckGeneralTags(doc);
}
}  // namespace