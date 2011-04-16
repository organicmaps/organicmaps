#include "classificator_loader.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"

#include "../coding/reader.hpp"
#include "../base/logging.hpp"

namespace classificator
{
  void Read(string const & rules, string const & classificator, string const & visibility)
  {
    LOG(LINFO, ("Reading drawing rules"));
    drule::ReadRules(rules.c_str());
    LOG(LINFO, ("Reading classificator"));
    if (!classif().ReadClassificator(classificator.c_str()))
      MYTHROW(Reader::OpenException, ("drawing rules or classificator file"));

    LOG(LINFO, ("Reading visibility"));
    (void)classif().ReadVisibility(visibility.c_str());
    LOG(LINFO, ("Reading visibility done"));
  }
}
