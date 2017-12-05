#include "indexer/indexer_tests_support/helpers.hpp"

#include "editor/editor_storage.hpp"

namespace indexer
{
namespace tests_support
{
void SetUpEditorForTesting(unique_ptr<osm::Editor::Delegate> delegate)
{
  auto & editor = osm::Editor::Instance();
  editor.SetDelegate(move(delegate));
  editor.SetStorageForTesting(make_unique<editor::InMemoryStorage>());
  editor.ClearAllLocalEdits();
}

void TearDownEditorForTesting()
{
  auto & editor = osm::Editor::Instance();
  editor.ClearAllLocalEdits();
  editor.SetDelegate({});
  editor.SetDefaultStorage();
}
}  // namespace tests_support
}  // namespace indexer
