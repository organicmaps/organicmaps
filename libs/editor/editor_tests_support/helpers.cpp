#include "editor/editor_tests_support/helpers.hpp"

#include "editor/editor_storage.hpp"

#include <utility>

namespace editor
{
namespace tests_support
{
void SetUpEditorForTesting(std::unique_ptr<osm::Editor::Delegate> delegate)
{
  auto & editor = osm::Editor::Instance();
  editor.SetDelegate(std::move(delegate));
  editor.SetStorageForTesting(std::make_unique<editor::InMemoryStorage>());
  editor.ClearAllLocalEdits();
  editor.ResetNotes();
}

void TearDownEditorForTesting()
{
  auto & editor = osm::Editor::Instance();
  editor.ClearAllLocalEdits();
  editor.ResetNotes();
  editor.SetDelegate({});
  editor.SetDefaultStorage();
}
}  // namespace tests_support
}  // namespace editor
