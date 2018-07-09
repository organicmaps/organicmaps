#include "testing/testing.hpp"

#include "editor/editor_config.hpp"
#include "editor/new_feature_categories.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "std/transform_iterator.hpp"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

UNIT_TEST(NewFeatureCategories_UniqueNames)
{
  using namespace std;

  classificator::Load();
  auto const & cl = classif();

  editor::EditorConfig config;
  osm::NewFeatureCategories categories(config);

  auto const & disabled = CategoriesHolder::kDisabledLanguages;

  bool noDuplicates = true;
  for (auto const & locale : CategoriesHolder::kLocaleMapping)
  {
    string const lang(locale.m_name);
    if (find(disabled.begin(), disabled.end(), lang) != disabled.end())
      continue;
    categories.AddLanguage(lang);
    auto const & names = categories.GetAllCategoryNames(lang);

    auto firstFn = bind(&pair<string, uint32_t>::first, placeholders::_1);
    set<string> uniqueNames(make_transform_iterator(names.begin(), firstFn),
                            make_transform_iterator(names.end(), firstFn));
    if (uniqueNames.size() == names.size())
      continue;

    LOG(LWARNING, ("Invalid category translations", lang));

    map<string, vector<uint32_t>> typesByName;
    for (auto const & entry : names)
      typesByName[entry.first].push_back(entry.second);

    for (auto const & entry : typesByName)
    {
      if (entry.second.size() <= 1)
        continue;
      noDuplicates = false;
      ostringstream str;
      str << entry.first << ":";
      for (auto const & type : entry.second)
        str << " " << cl.GetReadableObjectName(type);
      LOG(LWARNING, (str.str()));
    }

    LOG(LWARNING,
        ("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"));
  };

  TEST(noDuplicates, ());
}
