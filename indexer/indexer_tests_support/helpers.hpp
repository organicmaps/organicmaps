#pragma once

#include "indexer/editable_map_object.hpp"
#include "indexer/osm_editor.hpp"

#include "base/assert.hpp"

#include "std/unique_ptr.hpp"

namespace indexer
{
namespace tests_support
{
void SetUpEditorForTesting(unique_ptr<osm::Editor::Delegate> delegate);
void TearDownEditorForTesting();

template <typename TFn>
void EditFeature(FeatureType const & ft, TFn && fn)
{
  auto & editor = osm::Editor::Instance();

  osm::EditableMapObject emo;
  emo.SetFromFeatureType(ft);
  emo.SetEditableProperties(editor.GetEditableProperties(ft));

  fn(emo);

  CHECK_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());
}
}  // namespace tests_support
}  // namespace indexer
