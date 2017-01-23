#pragma once

#include "search/token_slice.hpp"

#include "indexer/categories_holder.hpp"

#include "base/buffer_vector.hpp"
#include "base/string_utils.hpp"

namespace search
{
using TLocales = buffer_vector<int8_t, 3>;

template <typename ToDo>
void ForEachCategoryType(StringSliceBase const & slice, TLocales const & locales,
                         CategoriesHolder const & categories, ToDo && todo)
{
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    auto token = slice.Get(i);
    for (size_t j = 0; j < locales.size(); ++j)
      categories.ForEachTypeByName(locales[j], token, bind<void>(todo, i, _1));

    // Special case processing of 2 codepoints emoji (e.g. black guy on a bike).
    // Only emoji synonyms can have one codepoint.
    if (token.size() > 1)
    {
      categories.ForEachTypeByName(CategoriesHolder::kEnglishCode, strings::UniString(1, token[0]),
                                   bind<void>(todo, i, _1));
    }
  }
}
}  // namespace search
