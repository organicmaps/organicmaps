#include "testing/testing.hpp"

#include "editor/editor_config.hpp"
#include "editor/new_feature_categories.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include <algorithm>
#include <string>

using namespace std;

UNIT_TEST(NewFeatureCategories_UniqueNames)
{
  classificator::Load();

  editor::EditorConfig config;
  osm::NewFeatureCategories categories(config);

  auto const & disabled = CategoriesHolder::kDisabledLanguages;

  for (auto const & locale : CategoriesHolder::kLocaleMapping)
  {
    string const lang(locale.m_name);
    if (find(disabled.begin(), disabled.end(), lang) != disabled.end())
      continue;
    categories.AddLanguage(lang);
    auto names = categories.GetAllCreatableTypeNames();
    sort(names.begin(), names.end());
    auto result = unique(names.begin(), names.end());

    if (result != names.end())
    {
      LOG(LWARNING, ("Types duplication detected! The following types are duplicated:"));
      do
      {
        LOG(LWARNING, (*result));
      } while (++result != names.end());

      TEST(false, ("Please look at output above"));
    }
  };
}
