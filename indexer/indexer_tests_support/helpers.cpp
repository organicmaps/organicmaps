#include "indexer/indexer_tests_support/helpers.hpp"

#include "editor/editor_storage.hpp"

#include "std/unique_ptr.hpp"

namespace indexer
{
namespace tests_support
{
void SetUpEditorForTesting(Index & index)
{
  auto & editor = osm::Editor::Instance();
  editor.SetIndex(index);
  editor.SetStorageForTesting(make_unique<editor::InMemoryStorage>());
  editor.ClearAllLocalEdits();
}
}  // namespace tests_support
}  // namespace indexer
