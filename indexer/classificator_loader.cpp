#include "classificator_loader.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"

#include "../coding/reader.hpp"

namespace classificator
{
  void Read(string const & rules, string const & classificator, string const & visibility)
  {
    drule::ReadRules(rules.c_str());
    if (!classif().ReadClassificator(classificator.c_str()))
      MYTHROW(Reader::OpenException, ("drawing rules or classificator file"));

    (void)classif().ReadVisibility(visibility.c_str());
  }
}
