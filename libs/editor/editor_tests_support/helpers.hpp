#pragma once

#include "editor/osm_editor.hpp"

#include "indexer/editable_map_object.hpp"

#include "base/assert.hpp"

#include <memory>

namespace editor
{
namespace tests_support
{
void SetUpEditorForTesting(std::unique_ptr<osm::Editor::Delegate> delegate);
void TearDownEditorForTesting();

template <typename Fn>
void EditFeature(FeatureType & ft, Fn && fn)
{
  auto & editor = osm::Editor::Instance();

  osm::EditableMapObject emo;
  emo.SetFromFeatureType(ft);
  emo.SetEditableProperties(editor.GetEditableProperties(ft));

  fn(emo);

  CHECK_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());
}
}  // namespace tests_support
}  // namespace editor
