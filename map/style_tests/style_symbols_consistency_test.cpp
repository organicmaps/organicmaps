#include "testing/testing.hpp"

#include "map/style_tests/helpers.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "coding/parse_xml.hpp"
#include "coding/reader.hpp"

#include <cstring>  // std::strcmp
#include <set>
#include <string>
#include <vector>

namespace style_symbols_consistency_tests
{
typedef std::set<std::string> StringSet;

class SdfParsingDispatcher
{
public:
  explicit SdfParsingDispatcher(StringSet & symbols)
    : m_symbols(symbols) {}

  void Load(const ReaderPtr<Reader> & source) const
  {
    std::string data;
    source.ReadAsString(data);
    auto it = data.begin();
    for (int i = 0; i < 3; i++)
      it = std::find(it, data.end(), '\n') + 1;
    while (it != data.end())
    {
      it = std::find(it, data.end(), '\n'); // EOL pos
      auto name_begin_it = std::find(std::make_reverse_iterator(it), std::make_reverse_iterator(data.begin()), ';').
          base();
      m_symbols.insert(std::string(name_begin_it, it));
      ++it;
    }
  }

private:
  StringSet & m_symbols;
};

StringSet GetSymbolsSetFromDrawingRule()
{
  StringSet symbols;
  drule::rules().ForEachRule([&symbols](drule::BaseRule const * rule)
  {
    SymbolRuleProto const * symbol = rule->GetSymbol();
    if (symbol && !symbol->name().empty())
      symbols.insert(symbol->name());
  });
  return symbols;
}

StringSet GetSymbolsSetFromResourcesFile(std::string_view density)
{
  StringSet symbols;
  SdfParsingDispatcher dispatcher(symbols);
  ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader("symbols.sdf", density);
  dispatcher.Load(reader);
  return symbols;
}

// Tests that all symbols specified in drawing rules have corresponding symbols in resources
UNIT_TEST(Test_SymbolsConsistency)
{
  bool res = true;

  std::string_view constexpr densities[] = {"mdpi", "hdpi", "xhdpi", "xxhdpi", "xxxhdpi", "6plus"};

  styles::RunForEveryMapStyle([&](MapStyle mapStyle)
  {
    StringSet const drawingRuleSymbols = GetSymbolsSetFromDrawingRule();

    for (std::string_view density : densities)
    {
      StringSet const resourceStyles = GetSymbolsSetFromResourcesFile(density);

      std::vector<std::string> missed;
      std::set_difference(drawingRuleSymbols.begin(), drawingRuleSymbols.end(),
                          resourceStyles.begin(), resourceStyles.end(),
                          back_inserter(missed));

      if (!missed.empty())
      {
        // We are interested in all set of bugs, therefore we do not stop test here but
        // continue it just keeping in res that test failed.
        LOG(LINFO, ("Symbols mismatch: style", mapStyle, ", density", density, ", missed", missed));
        res = false;
      }
    }
  });

  TEST(res, ());
}
} // namespace style_symbols_consistency_tests
